# engine_misc: true prototypes of the functions this module defines (docs/ARCHITECTURE.md 11)
# <addr> <prototype>;

# BGUI (debug menu GUI)
00047370 struct BGuiMenu *bGuiCreateMenu(unsigned int flags);
000473f0 void bGuiDeleteMenu(struct BGuiMenu *menu);
00047460 void bGuiSetRootMenu(struct BGuiMenu *menu);
00047480 struct BGuiItem *bGuiAddItem(struct BGuiMenu *menu, const char *name, unsigned short id, struct BGuiMenu *submenu, unsigned short flags);
00047520 void bGuiDrawPopup(struct BGuiMenu *menu, int x, int y);
00047830 void bGuiDrawMenuBar(struct BGuiMenu *bar);
00047970 void bGuiProcessInput(void);
00047cb0 void bGuiUpdate(void);
00047d20 void bGuiRemoveItem(struct BGuiMenu *menu, struct BGuiItem *item);
00047d80 struct BGuiItem *bGuiGetItem(struct BGuiMenu *menu, int index);
00047da0 struct BGuiItem *bGuiFindItem(struct BGuiMenu *menu, unsigned int id);
00047de0 void bGuiShutdown(void);
00047e30 void bGuiBindDefaultKeys(void);
00047f30 void bGuiLockScreen(void);
00047f80 void bGuiUnlockScreen(void);
00047f90 void bGuiDrawIcon(int icon, int x, int y);
000480d0 void bGuiDrawText(struct BGuiFont *font, int x, int y, const char *text);
00048310 void bGuiFillRect(int x, int y, int w, int h, int r, unsigned int g, unsigned int b);
00048380 void bGuiDrawBox(int x, int y, int w, int h, int style, int r, unsigned int g, unsigned int b);
00048790 void bGuiDrawSeparator(int x, int y, int w, int r, int g, int b);

# BINPUT
00048880 void biInit(int param);
000488c0 struct BInputMap *biCreateMap(int maxChannels);
