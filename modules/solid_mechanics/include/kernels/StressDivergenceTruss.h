#ifndef STRESSDIVERGENCETRUSS_H
#define STRESSDIVERGENCETRUSS_H

#include "Kernel.h"

//Forward Declarations
class ColumnMajorMatrix;
class SymmElasticityTensor;
class SymmTensor;

class StressDivergenceTruss : public Kernel
{
public:

  StressDivergenceTruss(const std::string & name, InputParameters parameters);
  virtual ~StressDivergenceTruss() {}

protected:
  virtual void computeResidual();
  virtual Real computeQpResidual() {return 0;}

  virtual void computeJacobian();

  virtual void computeOffDiagJacobian(unsigned int jvar);

  void computeStiffness(ColumnMajorMatrix & stiff_global);

  MaterialProperty<Real> & _axial_stress;
  MaterialProperty<Real> & _E_over_L;

private:
  const unsigned int _component;

  const bool _xdisp_coupled;
  const bool _ydisp_coupled;
  const bool _zdisp_coupled;
  const bool _temp_coupled;

  const unsigned int _xdisp_var;
  const unsigned int _ydisp_var;
  const unsigned int _zdisp_var;
  const unsigned int _temp_var;

  const VariableValue & _area;
  const std::vector<RealGradient> & _orientation;

};

template<>
InputParameters validParams<StressDivergenceTruss>();

#endif //STRESSDIVERGENCETRUSS_H
