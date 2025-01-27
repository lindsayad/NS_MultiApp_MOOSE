//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "FVNavStokesPressurePredictor_p.h"
#include "MooseMesh.h"

registerMooseObject("AirfoilAppApp", FVNavStokesPressurePredictor_p);

InputParameters
FVNavStokesPressurePredictor_p::validParams()
{

  auto params = FVFluxKernel::validParams();

  params.addClassDescription("Object pressure predictor equation.");
  // Coupled variables
  //params.addRequiredCoupledVar("pressure", "The pressure variable.");
  // params.addRequiredCoupledVar("pressure_old", "The pressure old variable.");
  // params.addRequiredCoupledVar("u_star", "The velocity in the x direction.");
  // params.addCoupledVar("v_star", "The velocity in the y direction.");
  // params.addCoupledVar("w_star", "The velocity in the z direction.");
  params.addRequiredCoupledVar("Ainv_x", "Ainv_x from momenutm predictor.");
  params.addCoupledVar("Ainv_y", "Ainv_y from momenutm predictor.");
  params.addCoupledVar("Ainv_z", "Ainv_z from momenutm predictor.");
  params.addRequiredCoupledVar("Hu_x", "Hu_x from momenutm predictor.");
  params.addCoupledVar("Hu_y", "Hu_y from momenutm predictor.");
  params.addCoupledVar("Hu_z", "Hu_z from momenutm predictor.");
  // params.addRequiredCoupledVar("rhs_x", "rhs_x from momenutm predictor.");
  // params.addCoupledVar("rhs_y", "rhs_y from momenutm predictor.");
  // params.addCoupledVar("rhs_z", "rhs_z from momenutm predictor.");

  // Set velocity interpolation method for the RSH term
  // MooseEnum velocity_interp_method("average rc", "rc");
  //
  // params.addParam<MooseEnum>(
  //     "velocity_interp_method",
  //     velocity_interp_method,
  //     "The interpolation to use for the velocity. Options are "
  //     "'average' and 'rc' which stands for Rhie-Chow. The default is Rhie-Chow.");

  return params;
}

FVNavStokesPressurePredictor_p::FVNavStokesPressurePredictor_p(const InputParameters & parameters)
  :
    // Base class
    FVFluxKernel(parameters),

    // Material properties
    // _rho(getFunctor<ADReal>("rho")),
    // _mu(getFunctor<ADReal>("mu")),

    // Get coupled variables
    // _p_var(dynamic_cast<const INSFVPressureVariable *>(getFieldVar("pressure", 0))),
    // _p_old(dynamic_cast<const INSFVPressureVariable *>(getFieldVar("pressure_old", 0))),
    // _u_star(dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("u_star", 0))),
    // _v_star(_mesh.dimension() >= 2 ? dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("v_star", 0)) : nullptr),
    // _w_star(_mesh.dimension() == 3 ? dynamic_cast<const INSFVVelocityVariable *>(getFieldVar("w_star", 0)) : nullptr),

    // Get coupled predictor variables
    _Ainv_x(dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Ainv_x", 0))),
    _Ainv_y(_mesh.dimension() >= 2 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Ainv_y", 0)) : nullptr),
    _Ainv_z(_mesh.dimension() >= 3 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Ainv_z", 0)) : nullptr),

    _Hu_x(dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Hu_x", 0))),
    _Hu_y(_mesh.dimension() >= 2 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Hu_y", 0)) : nullptr),
    _Hu_z(_mesh.dimension() >= 3 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("Hu_z", 0)) : nullptr),

    // _rhs_x(dynamic_cast<const MooseVariableFVReal *>(getFieldVar("rhs_x", 0))),
    // _rhs_y(_mesh.dimension() >= 2 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("rhs_y", 0)) : nullptr),
    // _rhs_z(_mesh.dimension() >= 3 ? dynamic_cast<const MooseVariableFVReal *>(getFieldVar("rhs_z", 0)) : nullptr),

    // Variable numberings
    // _u_vel_star_var_number(coupled("u_star")),
    // _v_vel_star_var_number(_mesh.dimension() >= 2 ? coupled("v_star") : libMesh::invalid_uint),
    // _w_vel_star_var_number(_mesh.dimension() == 3 ? coupled("w_star") : libMesh::invalid_uint),

    // Get current element
    _current_elem(_assembly.elem()),
    counter(0)

{
  std::cout << "Constructor OK" << std::endl;

}


ADReal
FVNavStokesPressurePredictor_p::computeQpResidual()
{

  /// Diffusion term
  using namespace Moose::FV;

  const Elem * const elem = &_face_info->elem();
  const Elem * const neighbor = _face_info->neighborPtr();

  ADRealVectorValue elem_Ainv(_Ainv_x->getElemValue(elem)); //* _face_info->elemVolume());
  if (_Ainv_y)
    elem_Ainv(1) = _Ainv_y->getElemValue(elem); //* _face_info->elemVolume();
  if (_Ainv_z)
    elem_Ainv(2) = _Ainv_z->getElemValue(elem); //* _face_info->elemVolume();

  mooseAssert((neighbor == &_face_info->elem()) || (neighbor == _face_info->neighborPtr()),
              "Surely the neighbor has to match one of the face information's elements, right?");

  ADRealVectorValue neighbor_Ainv(_Ainv_x->getNeighborValue(neighbor, *_face_info, elem_Ainv(0)));
                                   //* _face_info->neighborVolume());
  if (_Ainv_y)
    neighbor_Ainv(1) = _Ainv_y->getNeighborValue(neighbor, *_face_info, elem_Ainv(1));
                        //* _face_info->neighborVolume();
  if (_Ainv_z)
    neighbor_Ainv(2) = _Ainv_z->getNeighborValue(neighbor, *_face_info, elem_Ainv(2));
                       //* _face_info->neighborVolume();

  ADRealVectorValue interp_Ainv_face;
  Moose::FV::interpolate(Moose::FV::InterpMethod::Average,
                         interp_Ainv_face,
                         elem_Ainv,
                         neighbor_Ainv,
                         *_face_info,
                         true);

  ADRealVectorValue Ainv_gradp(interp_Ainv_face(0) * _var.adGradSln(*_face_info)(0));
  if (_Ainv_y)
    Ainv_gradp(1) = interp_Ainv_face(1) * _var.adGradSln(*_face_info)(1);
  if (_Ainv_z)
    Ainv_gradp(2) = interp_Ainv_face(2) * _var.adGradSln(*_face_info)(2);

  ADReal residual =  Ainv_gradp * _face_info->normal(); //* interp_Ainv_face;

  /// Divergence term
  // Interpolating H to the faces
  ADRealVectorValue elem_Hu(_Hu_x->getElemValue(elem));
  if (_Hu_y)
    elem_Hu(1) = _Hu_y->getElemValue(elem);
  if (_Hu_z)
    elem_Hu(2) = _Hu_z->getElemValue(elem);

  ADRealVectorValue neighbor_Hu(_Hu_x->getNeighborValue(neighbor, *_face_info, elem_Hu(0)));
  if (_Hu_y)
    neighbor_Hu(1) = _Hu_y->getNeighborValue(neighbor, *_face_info, elem_Hu(1));
  if (_Hu_z)
    neighbor_Hu(2) = _Hu_z->getNeighborValue(neighbor, *_face_info, elem_Hu(2));

  ADRealVectorValue interp_Hu_face;
  Moose::FV::interpolate(Moose::FV::InterpMethod::Average,
                         interp_Hu_face,
                         elem_Hu,
                         neighbor_Hu,
                         *_face_info,
                         true);

  // Computing the divergence term
  // auto Hu_div = interp_Hu_face(0) * _face_info->normal()(0);
  //
  // if (_mesh.dimension() >= 2)
  // {
  //   Hu_div += interp_Hu_face(1) * _face_info->normal()(1);
  // }
  // if (_mesh.dimension() >= 3)
  // {
  //   Hu_div += interp_Hu_face(2) * _face_info->normal()(2);
  // }

  residual += interp_Hu_face * _face_info->normal();

  //std::cout << "Dif term:" << (Ainv_gradp * _face_info->normal()).value() << std::endl;
  std::cout << "Src term x:" << (interp_Hu_face(0)).value() << std::endl;
  std::cout << "Src term y:" << (interp_Hu_face(1)).value() << std::endl;

  // if(_is_transient)
  //   residual -= (_rho(_current_elem)/_dt) * Hu_div;

  //return _var.adGradSln(*_face_info) * _face_info->normal() + 1.0;
  // std::cout << "This is my return: " <<
  //   1.0 * _var.adGradSln(*_face_info) * _face_info->normal() - 1.0 << std::endl;

  // auto tup = Moose::FV::determineElemOneAndTwo(*_face_info, _var);
  // const bool elem_one_is_fi_elem = std::get<2>(tup);
  // const Elem * const elem_one = std::get<0>(tup);
  // const Elem * const elem_two = std::get<1>(tup);
  // std::cout << "Elem one is: " << elem_one_is_fi_elem << std::endl;
  // std::cout << "Counter: " << counter << std::endl;
  // ADReal constant_term = elem_one_is_fi_elem ?
  //                       1.0 / _face_info->faceArea() / _face_info->faceCoord() / _face_info->elem().n_sides()
  //                       : 0.0;

  return residual;
}
