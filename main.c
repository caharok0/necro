#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

PSP_MODULE_INFO("Necro Morselli", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define W 480
#define H 272
#define TILE 16
#define MAPW 30
#define MAPH 14
#define MAX_ENEMIES 10
#define MAX_NPCS 8
#define SAVE_PATH "ms0:/PSP/SAVEDATA/NMPSP0001/SAVE.DAT"

enum { SCENE_TITLE, SCENE_MAP, SCENE_DIALOG, SCENE_BATTLE, SCENE_MENU, SCENE_QUESTS };
enum { MAP_CITY, MAP_FOREST, MAP_CHAPEL, MAP_CRYPT, MAP_SANCTUM };

typedef struct {
    int x, y;
    int hp, maxhp, sp, maxsp;
    int level, xp, gold, potions;
    int key, quest, map;
} Player;

typedef struct {
    int x, y, hp, maxhp, alive, type, reward;
} Enemy;

typedef struct {
    int map, x, y;
    const char *name, *line1, *line2;
    int quest;
} NPC;

typedef struct {
    int map, x, y, level, xp, gold, hp, sp, potions, key, quest;
    unsigned int magic;
} SaveData;

typedef struct { unsigned int color; float x, y, z; } Vertex;

static Player p = { 14, 8, 36, 36, 24, 24, 1, 0, 20, 3, 0, 0, MAP_CITY };
static Enemy enemies[MAX_ENEMIES];
static int enemyCount = 0;
static int scene = SCENE_TITLE;
static int titleChoice = 0;
static int menuChoice = 0;
static int dialogPage = 0;
static const NPC *dialogNpc = NULL;
static int battleIndex = -1;
static int battleChoice = 0;
static int questChoice = 0;
static unsigned int frame = 0;
static int running = 1;
static unsigned int prevButtons = 0;
static int messageTimer = 0;
static char message[128] = "THE FORGOTTEN PATH AWAITS.";
static SceCtrlData padNow;
static unsigned int __attribute__((aligned(16))) list[262144];

static const char *mapNames[] = {
    "OLD CITY", "BLACK FOREST", "OLD CHAPEL", "CRYPT", "HIDDEN SANCTUM"
};

static const char *mapData[5][MAPH] = {
{
"##############################","#............##..............#","#............##..............#",
"#..####......##......####....#","#..#..#..............#..#....#","#..#..#...N..........#..#....#",
"#.................#..........#","#.................#..........#","#.................#..........#",
"#................N...........#","#..####................####..#","#............................#",
"#............................#","##############################"
},
{
"##############################","#^^^^^^^..............^^^^^^^#","#^^^^^..................^^^^^#",
"#....####.......####.........#","#....#................#......#","#....#......N.........#......#",
"#............................#","#...........####.............#","#...........#..#.............#",
"#...........#..#.............#","#...........####.............#","#............................#",
"#^^^^^^^..............^^^^^^^#","##############################"
},
{
"##############################","#............................#","#....##########..............#",
"#....#........#..............#","#....#........#....N.........#","#....#........#..............#",
"#....#........######.........#","#....#.......................#","#....##########..............#",
"#............................#","#...............######.......#","#............................#",
"#............................#","##############################"
},
{
"##############################","#............................#","#..########################..#",
"#..#......................#..#","#..#....####..............#..#","#..#....#..#..............#..#",
"#..#....#..#......####....#..#","#..#....####......#..#....#..#",
"#..#..............#..#....#..#","#..#..............####....#..#",
"#..#......................#..#","#..########################..#",
"#............................#","##############################"
},
{
"##############################","#............................#","#..######...........######...#",
"#..#....#...........#....#...#","#..#....#....N......#....#...#","#..#....#...........#....#...#",
"#.......#.....####..#........#","#.......#.....#..#..#........#","#.......#######..####........#",
"#............................#","#.........##########.........#","#............................#",
"#............................#","##############################"
}
};

static const NPC npcs[MAX_NPCS] = {
 {MAP_CITY, 5,5, "WATCHER", "THE CHAPEL ROAD IS OPEN. TAKE THIS OLD KEY.", "THE FOREST REMEMBERS EVERY FOOTSTEP.", 0},
 {MAP_CITY,20,9, "BELL GIRL", "THE BELL RANG AGAIN. NO ONE LIVES THERE.", "IF YOU HEAR YOUR NAME BELOW, KEEP WALKING.", 1},
 {MAP_CHAPEL,19,4, "OLD MONK", "THE CRYPT HAS A HIDDEN SANCTUM.", "THE LAST DOOR OPENS ONLY FOR SOMEONE WHO FINISHES THE PATH.", 2},
 {MAP_FOREST,14,5, "TRAVELLER", "THE TREES ARE STRANGELY QUIET TONIGHT.", "A CREATURE BLOCKS THE EASTERN ROAD.", 1},
 {MAP_SANCTUM,13,4, "ARCHIVIST", "YOU REACHED THE FORGOTTEN SANCTUM.", "THE PATH IS COMPLETE. NOTHING HERE WAS EVER LOST BY ACCIDENT.", 3}
};

static unsigned int C(unsigned char r, unsigned char g, unsigned char b) {
    return 0xff000000u | ((unsigned int)b << 16) | ((unsigned int)g << 8) | r;
}
static int pressed(int b) { return (padNow.Buttons & b) && !(prevButtons & b); }
static void setMessage(const char *s) {
    strncpy(message, s, sizeof(message)-1); message[sizeof(message)-1] = 0; messageTimer = 150;
}
static void rect(float x, float y, float w, float h, unsigned int color) {
    Vertex v[2] = {{color,x,y,0},{color,x+w,y+h,0}};
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, NULL, v);
}
static const unsigned char font5x7[96][7] = {
#include "font5x7.inc"
};
static void text(int x, int y, const char *s, unsigned int color) {
    int ox=x;
    while (*s) {
        unsigned char ch=(unsigned char)*s++;
        if (ch=='\n') { y+=9; x=ox; continue; }
        if (ch<32 || ch>127) { x+=6; continue; }
        const unsigned char *g=font5x7[ch-32];
        for (int r=0;r<7;r++) for (int c=0;c<5;c++) if (g[r]&(1<<(4-c))) rect(x+c,y+r,1,1,color);
        x+=6;
    }
}

/* Compact pixel interpretation of the supplied Necro references. */
static const char *necroSprite[] = {
"........#...........",".......###..........",".....########.......","....############....",
"...###..##..###.....","..####..##..####....","..###...##...###....","..###..####..###....",
"...#############....","....###########.....",".....##PPP##........","....##PPPPP##.......",
"...###RRRRR###......","..##RRRRRRRRR##.....","..##RRRBBBBRRR##....","..##RRRBBBBRRR##....",
"...##RRRRRRR##......","....##BBBBB##.......","....##BBBBB##.......","...###BBBBB###......",
"..###..##.##..###...","..##...##.##...##...",".###...##.##...###..","...................."
};
static void drawNecro(int sx,int sy,int scale) {
    for (int y=0;y<24;y++) for (int x=0;x<20;x++) {
        char q=necroSprite[y][x]; unsigned int col=0;
        if(q=='#') col=C(8,9,15); else if(q=='B') col=C(25,28,72); else if(q=='R') col=C(105,27,35); else if(q=='P') col=C(232,232,223); else continue;
        rect(sx+x*scale,sy+y*scale,scale,scale,col);
    }
    rect(sx+7*scale,sy+6*scale,2*scale,2*scale,C(10,10,14));
    rect(sx+11*scale,sy+6*scale,2*scale,2*scale,C(10,10,14));
}

static int blocked(int map,int tx,int ty) {
    if(tx<0||ty<0||tx>=MAPW||ty>=MAPH) return 1;
    return mapData[map][ty][tx]=='#';
}
static void spawnEnemies(void) {
    enemyCount=0; memset(enemies,0,sizeof(enemies));
    if(p.map==MAP_FOREST) {
        enemies[enemyCount++]=(Enemy){15,6,20,20,1,0,10};
        enemies[enemyCount++]=(Enemy){23,10,24,24,1,0,12};
    } else if(p.map==MAP_CHAPEL) {
        enemies[enemyCount++]=(Enemy){15,7,28,28,1,0,14};
        enemies[enemyCount++]=(Enemy){23,9,26,26,1,1,18};
    } else if(p.map==MAP_CRYPT) {
        enemies[enemyCount++]=(Enemy){7,5,34,34,1,1,22};
        enemies[enemyCount++]=(Enemy){22,7,46,46,1,2,35};
        enemies[enemyCount++]=(Enemy){18,10,32,32,1,1,24};
    } else if(p.map==MAP_SANCTUM) {
        enemies[enemyCount++]=(Enemy){22,5,72,72,1,3,80};
    }
}
static void transition(int map,int x,int y) {
    p.map=map; p.x=x; p.y=y; spawnEnemies(); scene=SCENE_MAP; setMessage(mapNames[map]);
}
static void gainXP(int n) {
    p.xp+=n;
    while(p.xp >= p.level*30) {
        p.xp-=p.level*30; p.level++; p.maxhp+=8; p.maxsp+=5; p.hp=p.maxhp; p.sp=p.maxsp; setMessage("LEVEL UP!");
    }
}
static void saveGame(void) {
    sceIoMkdir("ms0:/PSP/SAVEDATA",0777); sceIoMkdir("ms0:/PSP/SAVEDATA/NMPSP0001",0777);
    SaveData s={p.map,p.x,p.y,p.level,p.xp,p.gold,p.hp,p.sp,p.potions,p.key,p.quest,0x4E4D3031};
    SceUID f=sceIoOpen(SAVE_PATH,PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(f>=0){sceIoWrite(f,&s,sizeof(s));sceIoClose(f);setMessage("GAME SAVED.");} else setMessage("SAVE FAILED.");
}
static void loadGame(void) {
    SaveData s; SceUID f=sceIoOpen(SAVE_PATH,PSP_O_RDONLY,0777);
    if(f>=0 && sceIoRead(f,&s,sizeof(s))==(int)sizeof(s) && s.magic==0x4E4D3031) {
        sceIoClose(f); p.map=s.map;p.x=s.x;p.y=s.y;p.level=s.level;p.xp=s.xp;p.gold=s.gold;p.hp=s.hp;p.sp=s.sp;p.potions=s.potions;p.key=s.key;p.quest=s.quest;
        spawnEnemies(); scene=SCENE_MAP; setMessage("GAME LOADED.");
    } else { if(f>=0)sceIoClose(f); setMessage("NO SAVE FOUND."); }
}
static int nearbyNPC(void) {
    for(int i=0;i<MAX_NPCS;i++) if(npcs[i].map==p.map && abs(p.x-npcs[i].x)<=1 && abs(p.y-npcs[i].y)<=1) return i;
    return -1;
}
static void interact(void) {
    int n=nearbyNPC();
    if(n>=0) { dialogNpc=&npcs[n]; dialogPage=0; scene=SCENE_DIALOG; return; }
    if(p.map==MAP_CITY && p.x>=27 && p.y==7) {
        if(p.key) transition(MAP_FOREST,2,7); else setMessage("THE GATE IS LOCKED. FIND THE WATCHER.");
    } else if(p.map==MAP_FOREST && p.x>=27 && p.y==6) {
        transition(MAP_CHAPEL,2,7);
    } else if(p.map==MAP_CHAPEL && p.x>=27 && p.y==9) {
        transition(MAP_CRYPT,4,4);
    } else if(p.map==MAP_CRYPT && p.x>=27 && p.y==10) {
        if(p.quest>=3) transition(MAP_SANCTUM,4,4); else setMessage("THE SEAL WILL NOT YIELD YET.");
    } else if(p.map==MAP_SANCTUM && p.quest>=3) {
        setMessage("THE FORGOTTEN PATH IS COMPLETE.");
    } else setMessage("NOTHING TO DO HERE.");
}
static void dialogNext(void) {
    if(!dialogNpc) return;
    if(dialogPage==0) {
        if(dialogNpc->quest==0 && !p.key) { p.key=1; p.quest=1; p.gold+=5; setMessage("OBTAINED: OLD KEY."); }
        else if(dialogNpc->quest==2 && p.quest<2) { p.quest=2; setMessage("QUEST UPDATED."); }
        else if(dialogNpc->quest==3 && p.quest<4) { p.quest=4; setMessage("THE PATH IS COMPLETE."); }
        dialogPage=1;
    } else scene=SCENE_MAP;
}
static void playerMove(int dx,int dy) {
    int nx=p.x+dx, ny=p.y+dy;
    if(!blocked(p.map,nx,ny)) { p.x=nx; p.y=ny; }
    for(int i=0;i<enemyCount;i++) if(enemies[i].alive && p.x==enemies[i].x && p.y==enemies[i].y) { battleIndex=i; battleChoice=0; scene=SCENE_BATTLE; return; }
}
static void finishBattle(Enemy *e) {
    e->alive=0; p.gold+=e->reward; gainXP(12+e->type*10); if(e->type==3) p.quest=3; scene=SCENE_MAP; setMessage(e->type==3?"THE SANCTUM GUARDIAN FALLS.":"ENEMY DEFEATED.");
}
static void battleAction(int a) {
    if(battleIndex<0 || battleIndex>=enemyCount) return;
    Enemy *e=&enemies[battleIndex]; if(!e->alive) {scene=SCENE_MAP;return;}
    int dmg=0;
    if(a==0) dmg=6+p.level*2;
    else if(a==1) { if(p.sp<6){setMessage("NOT ENOUGH SP.");return;} p.sp-=6; dmg=14+p.level*3; setMessage("SHADOW STEP!"); }
    else if(a==2) { if(p.potions<=0){setMessage("NO POTIONS.");return;} p.potions--; p.hp+=18; if(p.hp>p.maxhp)p.hp=p.maxhp; setMessage("POTION USED."); }
    else if(a==3) { scene=SCENE_MAP; return; }
    if(a<2) { e->hp-=dmg; if(e->hp<=0){finishBattle(e);return;} setMessage("HIT!"); }
    int enemyDamage=3+e->type*2; if(e->type==3) enemyDamage=8; p.hp-=enemyDamage;
    if(p.hp<=0){p.hp=p.maxhp/2;p.sp=p.maxsp/2;scene=SCENE_MAP;transition(p.map,2,2);setMessage("NECRO FELL... RECOVERED AT THE ROAD.");}
}
static void update(void) {
    sceCtrlPeekBufferPositive(&padNow,1);
    if(scene==SCENE_TITLE) {
        if(pressed(PSP_CTRL_UP)||pressed(PSP_CTRL_DOWN)) titleChoice^=1;
        if(pressed(PSP_CTRL_CROSS)) { if(titleChoice==0){p=(Player){14,8,36,36,24,24,1,0,20,3,0,0,MAP_CITY};spawnEnemies();scene=SCENE_MAP;setMessage("NIGHT. THE OLD CITY IS SILENT.");} else {loadGame();if(scene!=SCENE_MAP)scene=SCENE_TITLE;} }
    } else if(scene==SCENE_MAP) {
        if(pressed(PSP_CTRL_UP))playerMove(0,-1); if(pressed(PSP_CTRL_DOWN))playerMove(0,1); if(pressed(PSP_CTRL_LEFT))playerMove(-1,0); if(pressed(PSP_CTRL_RIGHT))playerMove(1,0);
        if(pressed(PSP_CTRL_CROSS))interact(); if(pressed(PSP_CTRL_TRIANGLE))scene=SCENE_MENU; if(pressed(PSP_CTRL_START))saveGame();
    } else if(scene==SCENE_DIALOG) {
        if(pressed(PSP_CTRL_CROSS)||pressed(PSP_CTRL_CIRCLE))dialogNext();
    } else if(scene==SCENE_BATTLE) {
        if(pressed(PSP_CTRL_UP))battleChoice=(battleChoice+3)%4; if(pressed(PSP_CTRL_DOWN))battleChoice=(battleChoice+1)%4;
        if(pressed(PSP_CTRL_CROSS))battleAction(battleChoice); if(pressed(PSP_CTRL_CIRCLE))scene=SCENE_MAP;
    } else if(scene==SCENE_MENU) {
        if(pressed(PSP_CTRL_UP))menuChoice=(menuChoice+3)%4; if(pressed(PSP_CTRL_DOWN))menuChoice=(menuChoice+1)%4;
        if(pressed(PSP_CTRL_CROSS)){if(menuChoice==0){saveGame();scene=SCENE_MAP;}else if(menuChoice==1){loadGame();}else if(menuChoice==2){scene=SCENE_QUESTS;}else scene=SCENE_MAP;}
        if(pressed(PSP_CTRL_CIRCLE))scene=SCENE_MAP;
    } else if(scene==SCENE_QUESTS) {
        if(pressed(PSP_CTRL_CIRCLE)||pressed(PSP_CTRL_TRIANGLE))scene=SCENE_MENU;
    }
    prevButtons=padNow.Buttons;
}
static void drawHeader(void) {
    rect(0,0,W,28,C(7,8,13)); char h[160];
    snprintf(h,sizeof(h),"%s  LV %d  HP %d/%d  SP %d/%d  G %d",mapNames[p.map],p.level,p.hp,p.maxhp,p.sp,p.maxsp,p.gold);
    text(6,5,h,C(230,230,220));
}
static void drawMap(void) {
    rect(0,0,W,H,C(12,13,21));
    for(int y=0;y<MAPH;y++) for(int x=0;x<MAPW;x++) {
        char c=mapData[p.map][y][x]; unsigned int col;
        if(c=='#') col=C(31,31,40); else if(c=='^') col=C(16,32,25); else if(p.map==MAP_CITY) col=C(38,35,42); else if(p.map==MAP_FOREST) col=C(24,38,29); else if(p.map==MAP_CHAPEL) col=C(48,44,43); else if(p.map==MAP_CRYPT) col=C(28,27,35); else col=C(32,31,39);
        rect(x*TILE,y*TILE+28,TILE,TILE,col); if(c=='#')rect(x*TILE+2,y*TILE+30,TILE-4,3,C(48,48,58));
    }
    for(int i=0;i<MAX_NPCS;i++) if(npcs[i].map==p.map) { rect(npcs[i].x*TILE+4,npcs[i].y*TILE+30,8,12,C(155,118,74)); }
    for(int i=0;i<enemyCount;i++) if(enemies[i].alive) { unsigned int ec=(enemies[i].type==3)?C(110,25,45):C(90,90,100); rect(enemies[i].x*TILE+3,enemies[i].y*TILE+32,10,10,ec); rect(enemies[i].x*TILE+5,enemies[i].y*TILE+34,2,2,C(230,230,220)); }
    drawNecro(p.x*TILE-4,p.y*TILE+27,2); drawHeader();
    rect(0,252,W,20,C(7,8,13)); text(7,256,"D-PAD MOVE  X ACTION  TRI MENU  START SAVE",C(175,178,190));
    if(messageTimer>0){rect(10,205,460,38,C(7,8,13));text(18,211,message,C(225,225,220));messageTimer--;}
}
static void drawTitle(void) {
    rect(0,0,W,H,C(7,8,13)); drawNecro(205,35,3); text(122,132,"NECRO MORSELLI",C(235,235,225)); text(145,145,"THE FORGOTTEN PATH",C(150,155,180));
    text(178,178,titleChoice==0?"> NEW GAME":"  NEW GAME",C(225,225,220)); text(178,193,titleChoice==1?"> LOAD GAME":"  LOAD GAME",C(225,225,220)); text(144,232,"D-PAD  SELECT   X  ENTER",C(150,152,165));
}
static void drawDialog(void) {
    drawMap(); rect(18,171,444,78,C(5,6,10)); rect(18,171,444,1,C(145,145,155));
    text(28,180,dialogNpc?dialogNpc->name:"...",C(210,75,75)); text(28,198,dialogNpc?(dialogPage==0?dialogNpc->line1:dialogNpc->line2):"",C(225,225,220)); text(28,232,"X / O  CONTINUE",C(150,152,165));
}
static void drawBattle(void) {
    rect(0,0,W,H,C(9,10,16)); rect(0,0,W,38,C(7,8,13)); text(12,10,"BATTLE",C(230,230,220)); drawNecro(75,70,3);
    Enemy *e=&enemies[battleIndex]; rect(326,95,62,70,C(30,30,40)); rect(340,90,34,28,e->type==3?C(100,25,45):C(105,105,115)); rect(343,104,5,6,C(10,10,14)); rect(367,104,5,6,C(10,10,14));
    char a[100]; snprintf(a,sizeof(a),"ENEMY HP %d/%d",e->hp,e->maxhp); text(300,175,a,C(220,80,80)); snprintf(a,sizeof(a),"NECRO HP %d/%d SP %d/%d",p.hp,p.maxhp,p.sp,p.maxsp); text(20,175,a,C(220,220,220));
    rect(18,198,444,58,C(7,8,13)); const char *opts[]={"SHADOWLESS STRIKE","SHADOW STEP","POTION","RETREAT"};
    for(int i=0;i<4;i++) text(30,205+i*11,(i==battleChoice?"> ":"  "),C(220,220,220)), text(42,205+i*11,opts[i],i==battleChoice?C(235,225,210):C(155,158,170));
}
static void drawMenu(void) {
    drawMap(); rect(78,38,324,190,C(5,6,10)); text(105,55,"MENU",C(230,230,220));
    char a[80]; snprintf(a,sizeof(a),"LEVEL %d   XP %d",p.level,p.xp); text(105,76,a,C(220,220,220)); snprintf(a,sizeof(a),"POTIONS %d   GOLD %d",p.potions,p.gold); text(105,88,a,C(220,220,220));
    const char *opts[]={"SAVE GAME","LOAD GAME","QUEST LOG","BACK"}; for(int i=0;i<4;i++) text(110,118+i*17,i==menuChoice?"> ":"  ",C(220,220,220)),text(122,118+i*17,opts[i],i==menuChoice?C(235,225,210):C(155,158,170));
}
static void drawQuests(void) {
    rect(0,0,W,H,C(8,9,14)); text(28,24,"QUEST LOG",C(235,235,225));
    text(32,58,p.quest<1?"[ ] FIND THE WATCHER":"[X] FIND THE WATCHER",C(220,220,220));
    text(32,82,p.quest<2?"[ ] REACH THE OLD CHAPEL":"[X] REACH THE OLD CHAPEL",C(220,220,220));
    text(32,106,p.quest<3?"[ ] CROSS THE CRYPT":"[X] CROSS THE CRYPT",C(220,220,220));
    text(32,130,p.quest<4?"[ ] REACH THE HIDDEN SANCTUM":"[X] REACH THE HIDDEN SANCTUM",C(220,220,220));
    text(32,214,"O / TRI  BACK",C(150,152,165));
}
static int exitCallback(int arg1,int arg2,void *common){(void)arg1;(void)arg2;(void)common;running=0;return 0;}
static int CallbackThread(SceSize args,void *argp){(void)args;(void)argp;int cb=sceKernelCreateCallback("Exit Callback",exitCallback,NULL);if(cb>=0)sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
static int setupCallbacks(void){int th=sceKernelCreateThread("update_thread",CallbackThread,0x11,0xFA0,0,0);if(th>=0)sceKernelStartThread(th,0,0);return th;}

int main(void) {
    setupCallbacks(); sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    sceGuInit(); sceGuStart(GU_DIRECT,list); sceGuDrawBuffer(GU_PSM_8888,(void*)0,512); sceGuDispBuffer(W,H,(void*)0x88000,512); sceGuOffset(2048-(W/2),2048-(H/2)); sceGuViewport(2048,2048,W,H); sceGuDepthRange(0xc350,0x2710);
    sceGumMatrixMode(GU_PROJECTION); sceGumLoadIdentity(); sceGumOrtho(0,W,H,0,-1,1); sceGumMatrixMode(GU_VIEW); sceGumLoadIdentity(); sceGumMatrixMode(GU_MODEL); sceGumLoadIdentity(); sceGuDisable(GU_DEPTH_TEST); sceGuFinish(); sceGuSync(0,0); sceDisplayWaitVblankStart(); sceGuDisplay(GU_TRUE);
    spawnEnemies();
    while(running) {
        update(); sceGuStart(GU_DIRECT,list); sceGuClearColor(C(5,6,10)); sceGuClear(GU_COLOR_BUFFER_BIT); sceGuDisable(GU_TEXTURE_2D);
        if(scene==SCENE_TITLE)drawTitle(); else if(scene==SCENE_MAP)drawMap(); else if(scene==SCENE_DIALOG)drawDialog(); else if(scene==SCENE_BATTLE)drawBattle(); else if(scene==SCENE_MENU)drawMenu(); else drawQuests();
        sceGuFinish(); sceGuSync(0,0); sceDisplayWaitVblankStart(); sceGuSwapBuffers(); frame++;
    }
    sceGuTerm(); sceKernelExitGame(); return 0;
}
