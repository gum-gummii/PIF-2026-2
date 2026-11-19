#ifndef CUTSCENE_H
#define CUTSCENE_H

#include <raylib.h>

void Cutscene_StartBoss(Texture2D naraSheet, Rectangle naraFrame);
void Cutscene_Update(float dt);
void Cutscene_Draw(void);
void Cutscene_Stop(void);
bool Cutscene_IsActive(void);
void Cutscene_Unload(void);

#endif
