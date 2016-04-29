typedef char *Pchar;
typedef void *Pvoid;

typedef const Pchar CPchar;
typedef const void *CPvoid;

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif

#define STRUCT_STATIC static

#define CAST(a, b) (static_cast<(a)>(b))

#if __cplusplus >= 201103L
static thread_local void *TLS;
#define HAS_STD_TLS 1
#endif

#else
#include <stdbool.h>
#define STRUCT_STATIC
#define CAST(a, b) ((a)(b))

#if __STDC_VERSION__ >= 201112L
static _Thread_local void *TLS;
#define HAS_STD_TLS 1
#endif

#endif

#if !defined HAS_STD_TLS
#if __GNUC__ >= 4
static __thread void *TLS;
#else
#error "Don't know how to declare thread-local data with this compiler"
#endif
#else
#undef HAS_STD_TLS
#endif

static int add_c_function(lua_State *L) {
   lua_CFunction f = CAST(lua_CFunction, lua_touserdata(L, -1));
   lua_pushcfunction(L, f);
   lua_settable(L, LUA_REGISTRYINDEX);
   return 0;
}

#define TRAMPOLINE(rettype, name, retcount, ...)                               \
   static int trampoline_##name(lua_State *L) {                                \
      struct Struct_##name *args = CAST(struct Struct_##name *, TLS);          \
      rettype retval = name(__VA_ARGS__);                                      \
      int number_return_values = (retcount);                                   \
      TLS = CAST(void *, retval);                                              \
      return number_return_values;                                             \
   }

#define VOID_TRAMPOLINE(_, name, retcount, ...)                                \
   static int trampoline_##name(lua_State *L) {                                \
      struct Struct_##name *args = CAST(struct Struct_##name *, TLS);          \
      name(__VA_ARGS__);                                                       \
      return (retcount);                                                       \
   }

#define RETCAST_VOID(rettype, value)
#define RETCAST_VALUE CAST

static int get_popped(const char *str) {
   bool seen_f = false, seen_L = false;
   while (true) {
      switch (*str) {
         case 'L': seen_L = true; break;
         case 'f': seen_f = true; break;
         case '\0': return seen_L + seen_f;
      }
      str++;
   }
}


#define LOCAL_STRUCT(name, ...)                                                \
   struct Struct_##name local = {__VA_ARGS__};                                 \
   do { TLS = CAST(void *, &local); } while(0)

#define DUMMY_LOCAL_STRUCT(name, ...) do { } while(0)

#define EMIT_WRAPPER(rettype, name, argcount, LOCAL_STRUCT, RETCAST, ...)      \
   /* Yes this is ugly, but it also makes the generating Lua script            \
    * MUCH simpler.  So this ugly long macro stays.                            \
    *                                                                          \
    * Ideally the C preprocessor would have loops like M4, but                 \
    * it doesn't,                                                              \
    */                                                                         \
   rettype safe_##name##_(int *success, lua_State *L ARGLIST) __attribute__((visibility("default"))) { \
      LOCAL_STRUCT(name, __VA_ARGS__);                                         \
      *success = 0;                                                            \
      lua_pushlightuserdata(L, (void*)&trampoline_##name);                     \
      lua_rawget(L, (LUA_REGISTRYINDEX));                                      \
      if (lua_isnil(L, -1)) {                                                  \
         int x =                                                               \
             lua_cpcall(L, &add_c_function, CAST(void *, &trampoline_##name)); \
         if (x) {                                                              \
            *success = x;                                                      \
            return RETCAST(rettype, 0);                                        \
         }                                                                     \
         lua_pushlightuserdata(L, (void*)&trampoline_##name);                  \
         lua_rawget(L, (LUA_REGISTRYINDEX));                                   \
      }                                                                        \
      if (0 != (*success = lua_pcall(L, (argcount), (LUA_MULTRET), 0))) {      \
         return RETCAST(rettype, 0);                                           \
      }                                                                        \
      return RETCAST(rettype, TLS);                                            \
   }

#if 0
{
#elif defined __cplusplus
}
#endif