#ifndef CURVE_GLOBAL_H
#define CURVE_GLOBAL_H

// from https://gcc.gnu.org/wiki/Visibility
#if defined _WIN32 || defined __CYGWIN__
  #ifdef LIBCURVE25519_LIBRARY
    #ifdef __GNUC__
      #define LIBCURVE_DLL __attribute__ ((dllexport))
    #else
      #define LIBCURVE_DLL __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define LIBCURVE_DLL __attribute__ ((dllimport))
    #else
      #define LIBCURVE_DLL __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define LIBCURVE_DLL __attribute__ ((visibility ("default")))
  #else
    #define LIBCURVE_DLL
  #endif
#endif

#endif // CURVE_GLOBAL_H
