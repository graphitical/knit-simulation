#pragma once

#include <vector>
#include <functional>

#include <Eigen/Core>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>
#include <ctpl_stl.h>

#include "./macros.h"
#include "./Helper.h"

namespace simulator {

enum ConstraintType {
  UNDEF = 0,
  POINT,
  LENGTH,
  PBC
};

class Constraints {
public:
  using Referrer = std::function<double& (int, int)>;
  using Func = std::function<double(const Eigen::MatrixXd&)>;
  using JacobianFunc = std::function<void(const Eigen::MatrixXd&, const Referrer&)>;

  typedef std::pair<size_t, size_t> IndexPair;
  struct Entry {
    Func f;
    JacobianFunc fD;
    IndexPair indexes;
    ConstraintType type;
  };

  // Construct a constraint container with m control points.
  // To be called by Simulator only.
  Constraints(size_t m_, ctpl::thread_pool* thread_pool_) : m(m_), thread_pool(thread_pool_) { }

  // Adds constraint
  // f: constrain function of q
  // fD: d(f)/d(q(index)) for every index with non-zero value
  void addConstraint(Func f, JacobianFunc fD);
  // indexes: first is the index of the constraint, second is the index of a PBC constraint if it has one
  // Also tracks type
  void addConstraint(Func f, JacobianFunc fD, size_t index_1, size_t index_2, ConstraintType type);
  void addConstraint(Func f, JacobianFunc fD, size_t index, ConstraintType type);

  // Gets Jacobian matrix of size c x (3 * m)
  // where c is the number of constraints
  // evaluated at q.
  Eigen::SparseMatrix<double> getJacobian(const Eigen::MatrixXd& q) const;

  // Evaluates the constraints at q
  // Returns the matrix of size c x 1
  Eigen::MatrixXd calculate(const Eigen::MatrixXd& q) const;

  std::vector<Entry> getConstraintList() const { return constraints; }

private:
  // number of control points
  size_t m;

  // thread pool
  ctpl::thread_pool* thread_pool;
  
  // list of constraints
  std::vector<Entry> constraints;

};

} // namespace simulator