#ifndef GENERALGOODGRAPHICS_H
#define GENERALGOODGRAPHICS_H
 
void drawRectangle(int x, int y, int w, int h, int r, int g, int b, int a);
void endDrawing();
void fixCoords(int* _x, int* _y);
void initGraphics(int _windowWidth, int _windowHeight, int* _storeWindowWidth, int* _storeWindowHeight);
void setClearColor(int r, int g, int b, int a);
void startDrawing();
 
#endif /* GENERALGOODGRAPHICS_H */