/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

#include "DiscreteNucleationMap.h"
#include "MooseMesh.h"

// libmesh includes
#include "libmesh/quadrature.h"

template<>
InputParameters validParams<DiscreteNucleationMap>()
{
  InputParameters params = validParams<ElementUserObject>();
  params.addParam<Real>("radius", 0.0, "Radius for the inserted nuclei");
  params.addParam<Real>("int_width", 0.0, "Nucleus interface width for smooth nuclei");
  params.addRequiredParam<UserObjectName>("inserter", "DiscreteNucleationInserter user object");
  params.addCoupledVar("periodic", "Use the periodicity settings of this variable to populate the grain map");
  MultiMooseEnum setup_options(SetupInterface::getExecuteOptions());
  // the mapping needs to run at timestep begin, which is after the adaptivity
  // run of the previous timestep.
  setup_options = "timestep_begin";
  params.set<MultiMooseEnum>("execute_on") = setup_options;
  return params;
}

DiscreteNucleationMap::DiscreteNucleationMap(const InputParameters & parameters) :
    ElementUserObject(parameters),
    _mesh_changed(false),
    _inserter(getUserObject<DiscreteNucleationInserter>("inserter")),
    _periodic(isCoupled("periodic") ? coupled("periodic") : -1),
    _radius(getParam<Real>("radius")),
    _int_width(getParam<Real>("int_width")),
    _nucleus_list(_inserter.getNucleusList())
{
  _zero_map.assign(_fe_problem.getMaxQps(), 0.0);
}

void
DiscreteNucleationMap::initialize()
{
  if (_inserter.isMapUpdateRequired() || _mesh_changed)
  {
    _rebuild_map = true;
    _nucleus_map.clear();
  }
  else
    _rebuild_map = false;

  _mesh_changed = false;
}

void
DiscreteNucleationMap::execute()
{
  if (_rebuild_map)
  {
    // reserve space for each quadrature point in the element
    _elem_map.assign(_qrule->n_points(), 0);

    // store a random number for each quadrature point
    unsigned int active_nuclei = 0;
    for (unsigned int qp = 0; qp < _qrule->n_points(); ++qp)
    {
      Real r, rmin = std::numeric_limits<Real>::max();

      // find the distance to the closest nucleus
      for (unsigned i = 0; i < _nucleus_list.size(); ++i)
      {
        // use a non-periodic or periodic distance
        r = _periodic < 0 ?
              (_q_point[qp] - _nucleus_list[i].second).norm() :
              _mesh.minPeriodicDistance(_periodic, _q_point[qp], _nucleus_list[i].second);
        if (r < rmin)
          rmin = r;
      }

      // compute intensity value with smooth interface
      Real value = 0.0;
      if (rmin <= _radius - _int_width/2.0) //Inside circle
      {
        active_nuclei++;
        value = 1.0;
      }
      else if (rmin < _radius + _int_width/2.0) //Smooth interface
      {
        Real int_pos = (rmin - _radius + _int_width/2.0)/_int_width;
        active_nuclei++;
        value = (1.0 + std::cos(int_pos * libMesh::pi)) / 2.0;
      }
      _elem_map[qp] = value;
    }

    // if the map is not empty insert it
    if (active_nuclei > 0)
      _nucleus_map.insert(std::pair<dof_id_type, std::vector<Real> >(_current_elem->id(), _elem_map));
  }
}

void
DiscreteNucleationMap::threadJoin(const UserObject &y)
{
  // if the map needs to be updated we merge the maps from all threads
  if (_rebuild_map)
  {
    const DiscreteNucleationMap & uo = static_cast<const DiscreteNucleationMap &>(y);
    _nucleus_map.insert(uo._nucleus_map.begin(), uo._nucleus_map.end());
  }
}

void
DiscreteNucleationMap::meshChanged()
{
  _mesh_changed = true;
}

const std::vector<Real> &
DiscreteNucleationMap::nuclei(const Elem * elem) const
{
  NucleusMap::const_iterator i = _nucleus_map.find(elem->id());

  // if no entry in the map was found the element contains no nucleus
  if (i == _nucleus_map.end())
    return _zero_map;

  return i->second;
}
