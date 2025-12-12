/* newpaint_enhanced.c - 超級小畫家增強版 */
/* 基於原始 newpaint.c 擴展，加入多種新功能 */

#define NULL 0
#define LINE 1
#define RECTANGLE 2
#define TRIANGLE 3
#define DRAW_POINTS 4
#define DRAW_TEXT 5
#define POLYGON 6
#define CIRCLE 7
#define ERASER 8
#define SPRAY 9

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 函式宣告 */
void mouse(int, int, int, int);
void motion(int, int);
void key(unsigned char, int, int);
void display(void);
void drawSquare(int, int);
void myReshape(GLsizei, GLsizei);
void myinit(void);
void screen_box(int, int, int);
void right_menu(int);
void middle_menu(int);
void color_menu(int);
void pixel_menu(int);
void fill_menu(int);
void font_menu(int);
void tool_menu(int);
int pick(int, int);
void drawRubberRectangle(int, int, int, int);
void drawRubberCircle(int, int, int, int);
void saveDrawing(const char*);
void loadDrawing(const char*);
void undo(void);
void drawPolygon(void);
void deleteLastPolygon(void);
void drawSpray(int, int);
void drawEraser(int, int);
void colorPicker(int, int);

/* 全域變數 */
GLsizei wh = 600, ww = 800; /* 初始視窗大小 */
GLfloat size = 5.0;         /* 點的大小/橡皮擦大小 */
int draw_mode = 0;          /* 繪圖模式 */
int rx, ry;                 /* 游標位置 */

GLfloat r = 0.0, g = 0.0, b = 0.0; /* 繪圖顏色 */
int fill = 0;                      /* 填充標誌 */
int font_type = 0;                 /* 字型類型: 0=9x15, 1=TIMES, 2=HELVETICA */
int rubberband = 0;                /* 橡皮筋模式標誌 */
int start_x, start_y;              /* 橡皮筋起始點 */
int rubber_x, rubber_y;            /* 橡皮筋當前位置 */
int polygon_count = 0;             /* 多邊形點數計數 */
int polygon_points[100][2];        /* 多邊形點陣列 */
int eraser_mode = 0;               /* 橡皮擦模式 */
int spray_mode = 0;                /* 噴槍模式 */
int polygon_mode = 0;              /* 多邊形繪製模式 */

/* 繪製歷史記錄結構 */
#define MAX_HISTORY 20
typedef struct {
    int type;
    int x1, y1, x2, y2;
    float r, g, b;
    int filled;
    int points[100][2];
    int point_count;
} DrawingItem;

DrawingItem history[MAX_HISTORY];
int history_count = 0;

/* 繪製正方形 */
void drawSquare(int x, int y) {
    y = wh - y;
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    glVertex2f(x + size, y + size);
    glVertex2f(x - size, y + size);
    glVertex2f(x - size, y - size);
    glVertex2f(x + size, y - size);
    glEnd();
}

/* 噴槍效果 */
void drawSpray(int x, int y) {
    y = wh - y;
    glColor3f(r, g, b);
    glPointSize(1.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < 30; i++) {
        int dx = (rand() % (int)(size * 4)) - size * 2;
        int dy = (rand() % (int)(size * 4)) - size * 2;
        if (dx*dx + dy*dy <= size*size*4) {
            glVertex2i(x + dx, y + dy);
        }
    }
    glEnd();
    glPointSize(size);
}

/* 橡皮擦 */
void drawEraser(int x, int y) {
    y = wh - y;
    glColor3f(0.8, 0.8, 0.8);  /* 背景色 */
    glBegin(GL_POLYGON);
    glVertex2f(x + size, y + size);
    glVertex2f(x - size, y + size);
    glVertex2f(x - size, y - size);
    glVertex2f(x + size, y - size);
    glEnd();
}

/* 橡皮筋矩形 */
void drawRubberRectangle(int x1, int y1, int x2, int y2) {
    y1 = wh - y1;
    y2 = wh - y2;
    
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glColor3f(1.0, 1.0, 1.0);
    
    glBegin(GL_LINE_LOOP);
    glVertex2i(x1, y1);
    glVertex2i(x1, y2);
    glVertex2i(x2, y2);
    glVertex2i(x2, y1);
    glEnd();
    
    glLogicOp(GL_COPY);
    glDisable(GL_COLOR_LOGIC_OP);
    glFlush();
}

/* 橡皮筋圓形 */
void drawRubberCircle(int x1, int y1, int x2, int y2) {
    y1 = wh - y1;
    y2 = wh - y2;
    
    int radius = (int)sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glColor3f(1.0, 1.0, 1.0);
    
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; i++) {
        float angle = 2.0 * M_PI * i / 100.0;
        glVertex2f(x1 + radius * cos(angle), y1 + radius * sin(angle));
    }
    glEnd();
    
    glLogicOp(GL_COPY);
    glDisable(GL_COLOR_LOGIC_OP);
    glFlush();
}

/* 繪製多邊形 */
void drawPolygon(void) {
    if (polygon_count < 3) return;
    
    glColor3f(r, g, b);
    if (fill)
        glBegin(GL_POLYGON);
    else
        glBegin(GL_LINE_LOOP);
    
    for (int i = 0; i < polygon_count; i++) {
        glVertex2i(polygon_points[i][0], wh - polygon_points[i][1]);
    }
    glEnd();
    glFlush();
    
    /* 儲存到歷史記錄 */
    if (history_count < MAX_HISTORY) {
        history[history_count].type = POLYGON;
        history[history_count].r = r;
        history[history_count].g = g;
        history[history_count].b = b;
        history[history_count].filled = fill;
        history[history_count].point_count = polygon_count;
        for (int i = 0; i < polygon_count; i++) {
            history[history_count].points[i][0] = polygon_points[i][0];
            history[history_count].points[i][1] = polygon_points[i][1];
        }
        history_count++;
    }
    
    polygon_count = 0;
}

/* 刪除最後一個多邊形 */
void deleteLastPolygon(void) {
    /* 簡單實現：清除畫布並重繪所有歷史記錄除了最後一個 */
    if (history_count > 0) {
        history_count--;
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        /* 重繪所有工具列元素 */
        display();
        
        /* 重繪歷史記錄 */
        for (int i = 0; i < history_count; i++) {
            glColor3f(history[i].r, history[i].g, history[i].b);
            if (history[i].type == POLYGON) {
                if (history[i].filled)
                    glBegin(GL_POLYGON);
                else
                    glBegin(GL_LINE_LOOP);
                for (int j = 0; j < history[i].point_count; j++) {
                    glVertex2i(history[i].points[j][0], 
                              wh - history[i].points[j][1]);
                }
                glEnd();
            }
        }
        glFlush();
    }
}

/* 顏色選擇器 */
void colorPicker(int x, int y) {
    /* 簡單的顏色選擇器：點擊位置決定RGB值 */
    r = (float)x / ww;
    g = (float)(wh - y) / wh;
    b = 1.0 - (float)(x + y) / (ww + wh);
    
    /* 確保值在0-1之間 */
    if (r < 0.0) r = 0.0;
    if (r > 1.0) r = 1.0;
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    if (b < 0.0) b = 0.0;
    if (b > 1.0) b = 1.0;
}

/* 滑鼠移動處理（橡皮筋效果） */
void motion(int x, int y) {
    if (rubberband) {
        /* 清除之前的橡皮筋 */
        if (draw_mode == RECTANGLE) {
            drawRubberRectangle(start_x, start_y, rubber_x, rubber_y);
        } else if (draw_mode == CIRCLE) {
            drawRubberCircle(start_x, start_y, rubber_x, rubber_y);
        }
        
        /* 繪製新的橡皮筋 */
        if (draw_mode == RECTANGLE) {
            drawRubberRectangle(start_x, start_y, x, y);
        } else if (draw_mode == CIRCLE) {
            drawRubberCircle(start_x, start_y, x, y);
        }
        
        rubber_x = x;
        rubber_y = y;
    } else if (spray_mode) {
        drawSpray(x, y);
        glFlush();
    } else if (eraser_mode) {
        drawEraser(x, y);
        glFlush();
    }
}

/* 滑鼠事件處理 */
void mouse(int btn, int state, int x, int y) {
    static int count;
    int where;
    static int xp[2], yp[2];
    
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        where = pick(x, y);
        
        if (where != 0) {
            /* 只在模式切換時重置計數器 */
            if (draw_mode != where) {
                count = 0;
                draw_mode = where;
                polygon_count = 0;  /* 重置多邊形點數 */
                
                /* 設置特殊模式 */
                spray_mode = (where == SPRAY);
                eraser_mode = (where == ERASER);
                polygon_mode = (where == POLYGON);
                rubberband = 0;
            }
        } else {
            switch (draw_mode) {
                case (LINE):
                    if (count == 0) {
                        count++;
                        xp[0] = x;
                        yp[0] = y;
                    } else {
                        glColor3f(r, g, b);
                        glBegin(GL_LINES);
                        glVertex2i(x, wh - y);
                        glVertex2i(xp[0], wh - yp[0]);
                        glEnd();
                        glFlush();
                        count = 0;  /* 重置計數器，但保持在直線模式 */
                    }
                    break;
                    
                case (RECTANGLE):
                    if (count == 0) {
                        count++;
                        start_x = x;
                        start_y = y;
                        rubber_x = x;
                        rubber_y = y;
                        rubberband = 1;
                    } else {
                        rubberband = 0;
                        glColor3f(r, g, b);
                        if (fill)
                            glBegin(GL_POLYGON);
                        else
                            glBegin(GL_LINE_LOOP);
                        glVertex2i(x, wh - y);
                        glVertex2i(x, wh - start_y);
                        glVertex2i(start_x, wh - start_y);
                        glVertex2i(start_x, wh - y);
                        glEnd();
                        glFlush();
                        count = 0;  /* 重置計數器，但保持在矩形模式 */
                    }
                    break;
                    
                case (CIRCLE):
                    if (count == 0) {
                        count++;
                        start_x = x;
                        start_y = y;
                        rubber_x = x;
                        rubber_y = y;
                        rubberband = 1;
                    } else {
                        rubberband = 0;
                        int radius = (int)sqrt((x-start_x)*(x-start_x) + 
                                              (y-start_y)*(y-start_y));
                        glColor3f(r, g, b);
                        glBegin(fill ? GL_POLYGON : GL_LINE_LOOP);
                        for (int i = 0; i < 100; i++) {
                            float angle = 2.0 * M_PI * i / 100.0;
                            glVertex2f(start_x + radius * cos(angle), 
                                      wh - start_y + radius * sin(angle));
                        }
                        glEnd();
                        glFlush();
                        count = 0;  /* 重置計數器，但保持在圓形模式 */
                    }
                    break;
                    
                case (TRIANGLE):
                    switch (count) {
                        case (0):
                            count++;
                            xp[0] = x;
                            yp[0] = y;
                            break;
                        case (1):
                            count++;
                            xp[1] = x;
                            yp[1] = y;
                            break;
                        case (2):
                            glColor3f(r, g, b);
                            if (fill)
                                glBegin(GL_POLYGON);
                            else
                                glBegin(GL_LINE_LOOP);
                            glVertex2i(xp[0], wh - yp[0]);
                            glVertex2i(xp[1], wh - yp[1]);
                            glVertex2i(x, wh - y);
                            glEnd();
                            glFlush();
                            count = 0;  /* 重置計數器，但保持在三角形模式 */
                    }
                    break;
                    
                case (DRAW_POINTS):
                    /* 隨機顏色 */
                    glColor3f((float)rand()/RAND_MAX, (float)rand()/RAND_MAX, (float)rand()/RAND_MAX);
                    y = wh - y;
                    glBegin(GL_POLYGON);
                    glVertex2f(x + size, y + size);
                    glVertex2f(x - size, y + size);
                    glVertex2f(x - size, y - size);
                    glVertex2f(x + size, y - size);
                    glEnd();
                    glFlush();
                    break;
                    
                case (DRAW_TEXT):
                    rx = x;
                    ry = wh - y;
                    glRasterPos2i(rx, ry);
                    break;
                    
                case (POLYGON):
                    if (polygon_count < 100) {
                        polygon_points[polygon_count][0] = x;
                        polygon_points[polygon_count][1] = y;
                        polygon_count++;
                        
                        /* 繪製點 */
                        glColor3f(r, g, b);
                        glPointSize(3.0);
                        glBegin(GL_POINTS);
                        glVertex2i(x, wh - y);
                        glEnd();
                        glFlush();
                        
                        /* 如果點數足夠，繪製線段 */
                        if (polygon_count > 1) {
                            glBegin(GL_LINES);
                            glVertex2i(polygon_points[polygon_count-2][0], 
                                      wh - polygon_points[polygon_count-2][1]);
                            glVertex2i(x, wh - y);
                            glEnd();
                            glFlush();
                        }
                    }
                    break;
                    
                case (SPRAY):
                    drawSpray(x, y);
                    glFlush();
                    break;
                    
                case (ERASER):
                    drawEraser(x, y);
                    glFlush();
                    break;
            }
        }
    } else if (btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        /* 右鍵完成多邊形 */
        if (draw_mode == POLYGON && polygon_count >= 3) {
            drawPolygon();
            draw_mode = 0;
            polygon_mode = 0;
        }
        /* 右鍵顏色選擇 */
        else if (draw_mode == 0) {
            colorPicker(x, y);
        }
    }
}

/* 選擇工具列項目 */
int pick(int x, int y) {
    y = wh - y;
    if (y < wh - ww / 12)
        return 0;
    else if (x < ww / 12)
        return LINE;
    else if (x < 2 * ww / 12)
        return RECTANGLE;
    else if (x < 3 * ww / 12)
        return TRIANGLE;
    else if (x < 4 * ww / 12)
        return DRAW_POINTS;
    else if (x < 5 * ww / 12)
        return DRAW_TEXT;
    else if (x < 6 * ww / 12)
        return POLYGON;
    else if (x < 7 * ww / 12)
        return CIRCLE;
    else if (x < 8 * ww / 12)
        return ERASER;
    else if (x < 9 * ww / 12)
        return SPRAY;
    else
        return 0;
}

/* 儲存繪圖 */
void saveDrawing(const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (fp) {
        /* 儲存歷史記錄 */
        fwrite(&history_count, sizeof(int), 1, fp);
        fwrite(history, sizeof(DrawingItem), history_count, fp);
        fclose(fp);
        printf("繪圖已儲存至 %s\n", filename);
    }
}

/* 載入繪圖 */
void loadDrawing(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp) {
        /* 清除當前畫布 */
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        /* 讀取歷史記錄 */
        fread(&history_count, sizeof(int), 1, fp);
        fread(history, sizeof(DrawingItem), history_count, fp);
        fclose(fp);
        
        /* 重繪所有項目 */
        display();
        for (int i = 0; i < history_count; i++) {
            glColor3f(history[i].r, history[i].g, history[i].b);
            if (history[i].type == POLYGON) {
                if (history[i].filled)
                    glBegin(GL_POLYGON);
                else
                    glBegin(GL_LINE_LOOP);
                for (int j = 0; j < history[i].point_count; j++) {
                    glVertex2i(history[i].points[j][0], 
                              wh - history[i].points[j][1]);
                }
                glEnd();
            }
        }
        glFlush();
        printf("繪圖已從 %s 載入\n", filename);
    }
}

/* 復原 */
void undo(void) {
    if (history_count > 0) {
        history_count--;
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
        
        for (int i = 0; i < history_count; i++) {
            glColor3f(history[i].r, history[i].g, history[i].b);
            if (history[i].type == POLYGON) {
                if (history[i].filled)
                    glBegin(GL_POLYGON);
                else
                    glBegin(GL_LINE_LOOP);
                for (int j = 0; j < history[i].point_count; j++) {
                    glVertex2i(history[i].points[j][0], 
                              wh - history[i].points[j][1]);
                }
                glEnd();
            }
        }
        glFlush();
    }
}

/* 視窗大小調整 */
void myReshape(GLsizei w, GLsizei h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)w, 0.0, (GLdouble)h, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, w, h);
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    display();
    glFlush();

    ww = w;
    wh = h;
}

/* 初始化 */
void myinit(void) {
    glViewport(0, 0, ww, wh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)ww, 0.0, (GLdouble)wh, -1.0, 1.0);
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    srand(time(NULL));  /* 初始化隨機數生成器 */
}

/* 工具列方塊繪製 */
void screen_box(int x, int y, int s) {
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + s, y);
    glVertex2i(x + s, y + s);
    glVertex2i(x, y + s);
    glEnd();
}

/* 右鍵選單 */
void right_menu(int id) {
    if (id == 1)
        exit(0);
    else if (id == 2) {
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
        glFlush();
        history_count = 0;  /* 清除歷史記錄 */
    } else if (id == 3) {
        saveDrawing("drawing.bin");
    } else if (id == 4) {
        loadDrawing("drawing.bin");
    } else if (id == 5) {
        undo();
    } else if (id == 6) {
        deleteLastPolygon();
    }
}

/* 中間鍵選單 */
void middle_menu(int id) {
    /* 空實現，可擴展 */
}

/* 顏色選單 */
void color_menu(int id) {
    if (id == 1) { r = 1.0; g = 0.0; b = 0.0; }
    else if (id == 2) { r = 0.0; g = 1.0; b = 0.0; }
    else if (id == 3) { r = 0.0; g = 0.0; b = 1.0; }
    else if (id == 4) { r = 0.0; g = 1.0; b = 1.0; }
    else if (id == 5) { r = 1.0; g = 0.0; b = 1.0; }
    else if (id == 6) { r = 1.0; g = 1.0; b = 0.0; }
    else if (id == 7) { r = 1.0; g = 1.0; b = 1.0; }
    else if (id == 8) { r = 0.0; g = 0.0; b = 0.0; }
    else if (id == 9) { r = 0.5; g = 0.5; b = 0.5; }
    else if (id == 10) { r = 1.0; g = 0.5; b = 0.0; }
}

/* 像素大小選單 */
void pixel_menu(int id) {
    if (id == 1 && size < 50.0) size *= 1.5;
    else if (id == 2 && size > 1.0) size /= 1.5;
}

/* 填充選單 */
void fill_menu(int id) {
    fill = (id == 1);
}

/* 字型選單 */
void font_menu(int id) {
    font_type = id - 1;
}

/* 工具選單 */
void tool_menu(int id) {
    draw_mode = id;
}

/* 鍵盤事件處理 */
void key(unsigned char k, int xx, int yy) {
    if (draw_mode == DRAW_TEXT) {
        glColor3f(r, g, b);
        glRasterPos2i(rx, ry);
        
        /* 根據選擇的字型繪製文字 */
        if (font_type == 0) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, k);
            rx += glutBitmapWidth(GLUT_BITMAP_9_BY_15, k);
        } else if (font_type == 1) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, k);
            rx += glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_10, k);
        } else if (font_type == 2) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, k);
            rx += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, k);
        }
        glFlush();
    } else if (k == 's' || k == 'S') {
        saveDrawing("drawing.bin");
    } else if (k == 'l' || k == 'L') {
        loadDrawing("drawing.bin");
    } else if (k == 'u' || k == 'U') {
        undo();
    } else if (k == 'd' || k == 'D') {
        deleteLastPolygon();
    } else if (k == 'c' || k == 'C') {
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
        glFlush();
        history_count = 0;
    }
}

/* 顯示函式 - 繪製工具列 */
void display(void) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    /* 繪製工具列背景 */
    glColor3f(0.6, 0.6, 0.6);
    glBegin(GL_QUADS);
    glVertex2i(0, wh - ww / 12);
    glVertex2i(ww, wh - ww / 12);
    glVertex2i(ww, wh);
    glVertex2i(0, wh);
    glEnd();
    
    /* 繪製工具按鈕 */
    int btn_width = ww / 12;
    int btn_height = ww / 12;
    int start_y = wh - btn_height;
    
    /* 線條按鈕 */
    glColor3f(1.0, 1.0, 1.0);
    screen_box(0, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINES);
    glVertex2i(btn_width/4, start_y + btn_height/2);
    glVertex2i(3*btn_width/4, start_y + btn_height/2);
    glEnd();
    
    /* 矩形按鈕 */
    glColor3f(1.0, 0.0, 0.0);
    screen_box(btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(btn_width + btn_width/4, start_y + btn_height/4);
    glVertex2i(btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(btn_width + 3*btn_width/4, start_y + btn_height/4);
    glEnd();
    
    /* 三角形按鈕 */
    glColor3f(0.0, 1.0, 0.0);
    screen_box(2*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex2i(2*btn_width + btn_width/2, start_y + btn_height/4);
    glVertex2i(2*btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(2*btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glEnd();
    
    /* 點按鈕 */
    glColor3f(0.0, 0.0, 1.0);
    screen_box(3*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex2i(3*btn_width + btn_width/2, start_y + btn_height/2);
    glEnd();
    
    /* 文字按鈕 */
    glColor3f(1.0, 1.0, 0.0);
    screen_box(4*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2i(4*btn_width + 5, start_y + 5);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'T');
    
    /* 多邊形按鈕 */
    glColor3f(0.5, 0.5, 1.0);
    screen_box(5*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(5*btn_width + btn_width/2, start_y + btn_height/4);
    glVertex2i(5*btn_width + btn_width/4, start_y + btn_height/2);
    glVertex2i(5*btn_width + btn_width/2, start_y + 3*btn_height/4);
    glVertex2i(5*btn_width + 3*btn_width/4, start_y + btn_height/2);
    glEnd();
    
    /* 圓形按鈕 */
    glColor3f(1.0, 0.5, 0.0);
    screen_box(6*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 20; i++) {
        float angle = 2.0 * M_PI * i / 20.0;
        glVertex2f(6*btn_width + btn_width/2 + btn_width/3*cos(angle),
                   start_y + btn_height/2 + btn_height/3*sin(angle));
    }
    glEnd();
    
    /* 橡皮擦按鈕 */
    glColor3f(0.8, 0.8, 0.8);
    screen_box(7*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_QUADS);
    glVertex2i(7*btn_width + btn_width/4, start_y + btn_height/4);
    glVertex2i(7*btn_width + 3*btn_width/4, start_y + btn_height/4);
    glVertex2i(7*btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(7*btn_width + btn_width/4, start_y + 3*btn_height/4);
    glEnd();
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_QUADS);
    glVertex2i(7*btn_width + btn_width/3, start_y + btn_height/3);
    glVertex2i(7*btn_width + 2*btn_width/3, start_y + btn_height/3);
    glVertex2i(7*btn_width + 2*btn_width/3, start_y + 2*btn_height/3);
    glVertex2i(7*btn_width + btn_width/3, start_y + 2*btn_height/3);
    glEnd();
    
    /* 噴槍按鈕 */
    glColor3f(0.5, 1.0, 0.5);
    screen_box(8*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(1.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < 10; i++) {
        int dx = rand() % (btn_width/2) - btn_width/4;
        int dy = rand() % (btn_height/2) - btn_height/4;
        glVertex2i(8*btn_width + btn_width/2 + dx,
                   start_y + btn_height/2 + dy);
    }
    glEnd();
    glPointSize(size);
    
    /* 顏色預覽 */
    glColor3f(r, g, b);
    screen_box(9*btn_width, start_y, btn_width);
    
    /* 狀態顯示 */
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2i(10, 20);
    char status[100];
    sprintf(status, "模式: %d 顏色: %.1f,%.1f,%.1f 大小: %.1f", 
            draw_mode, r, g, b, size);
    for (int i = 0; status[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, status[i]);
    }
    
    glFlush();
    glPopAttrib();
}

/* 主函式 */
int main(int argc, char **argv) {
    int c_menu, p_menu, f_menu, fo_menu, t_menu;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(ww, wh);
    glutCreateWindow("超級小畫家 - 期中考作業");
    
    /* 創建顏色選單 */
    c_menu = glutCreateMenu(color_menu);
    glutAddMenuEntry("紅色", 1);
    glutAddMenuEntry("綠色", 2);
    glutAddMenuEntry("藍色", 3);
    glutAddMenuEntry("青色", 4);
    glutAddMenuEntry("洋紅", 5);
    glutAddMenuEntry("黃色", 6);
    glutAddMenuEntry("白色", 7);
    glutAddMenuEntry("黑色", 8);
    glutAddMenuEntry("灰色", 9);
    glutAddMenuEntry("橙色", 10);
    
    /* 創建像素大小選單 */
    p_menu = glutCreateMenu(pixel_menu);
    glutAddMenuEntry("增加大小", 1);
    glutAddMenuEntry("減少大小", 2);
    
    /* 創建填充選單 */
    f_menu = glutCreateMenu(fill_menu);
    glutAddMenuEntry("填充開啟", 1);
    glutAddMenuEntry("填充關閉", 2);
    
    /* 創建字型選單 */
    fo_menu = glutCreateMenu(font_menu);
    glutAddMenuEntry("9x15 字型", 1);
    glutAddMenuEntry("Times Roman", 2);
    glutAddMenuEntry("Helvetica", 3);
    
    /* 創建右鍵選單 */
    glutCreateMenu(right_menu);
    glutAddMenuEntry("退出", 1);
    glutAddMenuEntry("清除", 2);
    glutAddMenuEntry("儲存", 3);
    glutAddMenuEntry("載入", 4);
    glutAddMenuEntry("復原", 5);
    glutAddMenuEntry("刪除多邊形", 6);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    /* 創建中鍵選單 */
    glutCreateMenu(middle_menu);
    glutAddSubMenu("顏色", c_menu);
    glutAddSubMenu("大小", p_menu);
    glutAddSubMenu("填充", f_menu);
    glutAddSubMenu("字型", fo_menu);
    glutAttachMenu(GLUT_MIDDLE_BUTTON);
    
    myinit();
    glutReshapeFunc(myReshape);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutDisplayFunc(display);
    glutMainLoop();
    
    return 0;
}