/**
 * @file
 * @author lkoppel
 */

#ifndef WAVE_GEOMETRY_VECTORBASE_HPP
#define WAVE_GEOMETRY_VECTORBASE_HPP

namespace wave {

/** Mixin representing an expression on @f[ \mathbb{R}^n @f]
 */
template <typename Derived>
struct VectorBase : ExpressionBase<Derived> {
 private:
    using Scalar = internal::scalar_t<Derived>;
    using OutputType = internal::plain_output_t<Derived>;

 public:
    static auto Random() -> OutputType {
        return ::wave::Random<OutputType>{}.eval();
    }

    /** Returns an expression representing a zero element*/
    static auto Zero() -> ::wave::Zero<OutputType> {
        return ::wave::Zero<OutputType>{};
    }

    /** Returns a differentiable expression for the vector's L2 norm */
    auto norm() const & {
        return Norm<internal::arg_t<Derived &>>{this->derived()};
    }

    WAVE_OVERLOAD_SELF_METHOD_FOR_RVALUE(norm, Norm, Derived)

    /** Returns a differentiable expression for the vector's squared L2 norm */
    auto squaredNorm() const & {
        return SquaredNorm<internal::arg_t<Derived &>>{this->derived()};
    }

    WAVE_OVERLOAD_SELF_METHOD_FOR_RVALUE(squaredNorm, SquaredNorm, Derived)

    /** Fuzzy comparison - see Eigen::DenseBase::isApprox() */
    template <typename R, TICK_REQUIRES(internal::same_base_tmpl_i<Derived, R>{})>
    bool isApprox(
      const VectorBase<R> &rhs,
      const Scalar &prec = Eigen::NumTraits<Scalar>::dummy_precision()) const {
        return this->eval().value().isApprox(rhs.eval().value(), prec);
    }

    /** True if approximately zero vector - see Eigen::DenseBase::isZero() */
    bool isZero(const Scalar &prec = Eigen::NumTraits<Scalar>::dummy_precision()) const {
        return this->eval().value().isZero(prec);
    }
};

namespace internal {

/** Base for traits of a vector leaf expression using an Eigen type for storage.*/
template <typename Derived>
struct vector_leaf_traits_base;

template <template <typename...> class Tmpl, typename ImplType_>
struct vector_leaf_traits_base<Tmpl<ImplType_>> : leaf_traits_base<Tmpl<ImplType_>>,
                                                  frameable_vector_traits {
    // Vector-specific traits
    using ImplType = ImplType_;

    template <typename NewImplType>
    using rebind = Tmpl<NewImplType>;

    // Overrides of universal leaf traits
    using Scalar = typename ImplType::Scalar;
    using PlainType = Tmpl<typename ImplType::PlainObject>;
    enum : int { Size = ImplType::SizeAtCompileTime, TangentSize = Size };
};

/** Helper to construct a vector expression given a leaf of the same kind */
template <typename OtherLeaf, typename ImplType>
auto makeVectorLike(ImplType &&arg) {
    return typename traits<OtherLeaf>::template rebind<tmp::remove_cr_t<ImplType>>{
      std::forward<ImplType>(arg)};
}

/** Implementation of conversion between two vector leaves
 *
 * While this seems trivial, it is needed for the case the template params are not the
 * same.
 */
template <typename ToType,
          typename Rhs,
          TICK_REQUIRES(internal::same_base_tmpl<ToType, Rhs>{})>
auto evalImpl(expr<Convert, ToType>, const VectorBase<Rhs> &rhs) -> ToType {
    return ToType{rhs.derived().value()};
}

/** Implementation of Sum for two vector leaves
 *
 * Since the temporary is so small, we return a plain vector instead of an Eigen::Sum.
 */
template <typename Lhs, typename Rhs, TICK_REQUIRES(internal::same_base_tmpl<Lhs, Rhs>{})>
auto evalImpl(expr<Sum>, const VectorBase<Lhs> &lhs, const VectorBase<Rhs> &rhs) {
    return plain_output_t<Rhs>{lhs.derived().value() + rhs.derived().value()};
}

/** Implementation of Minus for a vector leaf
 */
template <typename Rhs>
auto evalImpl(expr<Minus>, const VectorBase<Rhs> &rhs) {
    return makeVectorLike<Rhs>(-rhs.derived().value());
}

/** Implementation of Random for a vector leaf
 *
 * Produces a random vector with coefficients from -1 to 1
 */
template <typename Leaf, typename Rhs>
auto evalImpl(expr<Random, Leaf>, const VectorBase<Rhs> &) {
    return Leaf{internal::traits<Leaf>::ImplType::Random()};
}

/** Implementation of squared L2 norm for a vector leaf
 */
template <typename Rhs>
auto evalImpl(expr<SquaredNorm>, const VectorBase<Rhs> &rhs) {
    return makeScalarResult(rhs.derived().value().squaredNorm());
}

/** Gradient of squared L2 norm */
template <typename Val, typename Rhs>
auto jacobianImpl(expr<SquaredNorm>, const Val &, const VectorBase<Rhs> &rhs)
  -> jacobian_t<Val, Rhs> {
    return 2 * rhs.derived().value();
}

/** Implementation of L2 norm for a vector leaf
 */
template <typename Rhs>
auto evalImpl(expr<Norm>, const VectorBase<Rhs> &rhs) {
    return makeScalarResult(rhs.derived().value().norm());
}

/** Gradient of L2 norm */
template <typename Val, typename Rhs>
auto jacobianImpl(expr<Norm>, const ScalarBase<Val> &norm, const VectorBase<Rhs> &rhs)
  -> jacobian_t<Val, Rhs> {
    return rhs.derived().value() / norm.derived().value();
}

/** Implementation of left scalar multiplication
 * Defer to the implementation type's arithmetic operators.
 */
template <typename Lhs, typename Rhs>
auto evalImpl(expr<Scale>, const ScalarBase<Lhs> &lhs, const VectorBase<Rhs> &rhs) {
    return makeVectorLike<Rhs>(lhs.derived().value() * rhs.derived().value());
}
/** Implementation of right scalar multiplication
 * Defer to the implementation type's arithmetic operators.
 */
template <typename Lhs, typename Rhs>
auto evalImpl(expr<ScaleR>, const VectorBase<Lhs> &lhs, const ScalarBase<Rhs> &rhs) {
    return makeVectorLike<Lhs>(lhs.derived().value() * rhs.derived().value());
}
/** Left Jacobian implementation for left scalar multiplication */
template <typename Res, typename Lhs, typename Rhs>
decltype(auto) leftJacobianImpl(expr<Scale>,
                                const Res &,
                                const ScalarBase<Lhs> &,
                                const VectorBase<Rhs> &rhs) {
    return rhs.derived().value();
}
/** Right Jacobian implementation for right scalar multiplication */
template <typename Res, typename Lhs, typename Rhs>
decltype(auto) rightJacobianImpl(expr<ScaleR>,
                                 const Res &,
                                 const VectorBase<Lhs> &lhs,
                                 const ScalarBase<Rhs> &) {
    return lhs.derived().value();
}
/** Left Jacobian implementation for right scalar multiplication */
template <typename Res, typename Lhs, typename Rhs>
auto leftJacobianImpl(expr<ScaleR>,
                      const Res &,
                      const VectorBase<Lhs> &,
                      const ScalarBase<Rhs> &rhs) -> jacobian_t<Res, Lhs> {
    return rhs.derived().value() * identity_t<Lhs>{};
}
/** Right Jacobian implementation for left scalar multiplication */
template <typename Res, typename Lhs, typename Rhs>
auto rightJacobianImpl(expr<Scale>,
                       const Res &,
                       const ScalarBase<Lhs> &lhs,
                       const VectorBase<Rhs> &) -> jacobian_t<Res, Rhs> {
    return lhs.derived().value() * identity_t<Rhs>{};
}

/** Implementation of right scalar division
 * Defer to the implementation type's arithmetic operations.
 */
template <typename Lhs, typename Rhs>
auto evalImpl(expr<ScaleDiv>, const VectorBase<Lhs> &lhs, const ScalarBase<Rhs> &rhs) {
    return makeVectorLike<Lhs>(lhs.derived().value() / rhs.derived().value());
}

/** Left Jacobian implementation for right scalar division */
template <typename Res, typename Lhs, typename Rhs>
auto leftJacobianImpl(expr<ScaleDiv>,
                      const Res &,
                      const VectorBase<Lhs> &,
                      const ScalarBase<Rhs> &rhs) -> jacobian_t<Res, Lhs> {
    return identity_t<Res>{} / rhs.derived().value();
}

/** Right Jacobian implementation for right scalar division */
template <typename Res, typename Lhs, typename Rhs>
auto rightJacobianImpl(expr<ScaleDiv>,
                       const Res &,
                       const VectorBase<Lhs> &lhs,
                       const ScalarBase<Rhs> &rhs) -> jacobian_t<Res, Rhs> {
    const auto rhs_squared = rhs.derived().value() * rhs.derived().value();
    return jacobian_t<Res, Rhs>{-lhs.derived().value() / rhs_squared};
}

}  // namespace internal

/** Applies vector addition to two vector expressions (of the same space)
 *
 * @f[ \mathbb{R}^n \times \mathbb{R}^n \to \mathbb{R}^n @f]
 */
template <typename L, typename R>
auto operator+(const VectorBase<L> &lhs, const VectorBase<R> &rhs) {
    return Sum<internal::arg_t<L &>, internal::arg_t<R &>>{lhs.derived(), rhs.derived()};
}

WAVE_OVERLOAD_FUNCTION_FOR_RVALUES(operator+, Sum, VectorBase, VectorBase)

/** Applies vector addition to two vector expressions (of the same space)
 *
 * @f[ \mathbb{R}^n \times \mathbb{R}^n \to \mathbb{R}^n @f]
 */
template <typename L, typename R>
auto operator-(const VectorBase<L> &lhs, const VectorBase<R> &rhs) {
    return lhs.derived() + (-rhs.derived());
}

// Overloads for rvalues
template <typename L, typename R>
auto operator-(VectorBase<L> &&lhs, const VectorBase<R> &rhs) {
    return std::move(lhs).derived() + (-rhs.derived());
}
template <typename L, typename R>
auto operator-(const VectorBase<L> &lhs, VectorBase<R> &&rhs) {
    return lhs.derived() + (-std::move(rhs).derived());
}
template <typename L, typename R>
auto operator-(VectorBase<L> &&lhs, VectorBase<R> &&rhs) {
    return std::move(lhs).derived() + (-std::move(rhs).derived());
}

/** Negates a vector expression
 *
 * @f[ \mathbb{R}^n \to \mathbb{R}^n @f]
 */
template <typename R>
auto operator-(const VectorBase<R> &rhs) {
    return Minus<internal::arg_t<R &>>{rhs.derived()};
}

WAVE_OVERLOAD_FUNCTION_FOR_RVALUE(operator-, Minus, VectorBase)

/** Left scalar multiplication of a vector expression
 *
 * @f[ \mathbb{R} \times \mathbb{R}^n \to \mathbb{R}^n @f]
 */
template <typename L, typename R>
auto operator*(const ScalarBase<L> &lhs, const VectorBase<R> &rhs) {
    return Scale<internal::arg_t<L &>, internal::arg_t<R &>>{lhs.derived(),
                                                             rhs.derived()};
}

WAVE_OVERLOAD_FUNCTION_FOR_RVALUES(operator*, Scale, ScalarBase, VectorBase)
WAVE_OVERLOAD_OPERATORS_FOR_SCALAR_LEFT(*, VectorBase)

/** Right scalar multiplication of a vector expression
 *
 * @f[ \mathbb{R}^n \times \mathbb{R} \to \mathbb{R}^n @f]
 */
template <typename L, typename R>
auto operator*(const VectorBase<L> &lhs, const ScalarBase<R> &rhs) {
    return ScaleR<internal::arg_t<L &>, internal::arg_t<R &>>{lhs.derived(),
                                                              rhs.derived()};
}

WAVE_OVERLOAD_FUNCTION_FOR_RVALUES(operator*, ScaleR, VectorBase, ScalarBase)
WAVE_OVERLOAD_OPERATORS_FOR_SCALAR_RIGHT(*, VectorBase)

/** Right scalar division of a vector expression by a scalar expression
 *
 * @f[ \mathbb{R}^n \times \mathbb{R} \to \mathbb{R}^n @f]
 */
template <typename L, typename R>
auto operator/(const VectorBase<L> &lhs, const ScalarBase<R> &rhs) {
    return ScaleDiv<internal::arg_t<L &>, internal::arg_t<R &>>{lhs.derived(),
                                                                rhs.derived()};
}

WAVE_OVERLOAD_FUNCTION_FOR_RVALUES(operator/, ScaleDiv, VectorBase, ScalarBase)
WAVE_OVERLOAD_OPERATORS_FOR_SCALAR_RIGHT(/, VectorBase)

}  // namespace wave

#endif  // WAVE_GEOMETRY_VECTORBASE_HPP
