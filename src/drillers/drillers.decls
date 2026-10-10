# src/drillers/drillers.decls - prototype overrides of the drillers module
# (docs/ARCHITECTURE.md, section 11).
# <addr> <prototype>;   thiscall/fastcall `this` is the first parameter.

# Drillers/CModule_Drillers.cpp
0010f830 void *CModuleDrillers_Ctor(void *self);
0010f860 void CModuleDrillers_Dtor(void *self);
0010f870 void *CModuleDrillers_DeletingDtor(void *self, unsigned int flags);
0010f890 int CModuleDrillers_Start(void *self, int variant);

# Drillers/CDrillers_Game.cpp (CGameMode_Drillers; inferred)
001114a0 void *CGameModeDrillers_Ctor(void *self);
