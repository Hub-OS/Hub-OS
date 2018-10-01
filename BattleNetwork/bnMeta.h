#pragma once
/*
  Specialized templates (aka Metaprogramming), directives, and implementations used in the project
 */

 /*
 Define constraints on template in compile-time. If it fails the compiler will hault with a nice message.
 Silence if passes.
 */
template<class Any, class Base> struct _DerivedFrom {
  static void constraints(Any* p) { Base* pb = p; }
  _DerivedFrom() { /*void(*p)(Any*) = constraints;*/ }
};
