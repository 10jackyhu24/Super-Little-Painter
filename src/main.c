/* newpaint_enhanced.c - Super Paint Enhanced Version */
/* Based on original newpaint.c, with many new features */

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
#define FILL_BUCKET 10
#define EYEDROPPER 11
#define SELECT 12

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
void redo(void);
void drawPolygon(void);
void deleteLastPolygon(void);
void drawSpray(int, int);
void drawEraser(int, int);
void colorPicker(int, int);
void redrawHistory(void);
void floodFill(int, int, float, float, float);
void floodFillStack(int, int, float, float, float);
void scanlineFill(int, int, float, float, float);

/* Global Variables */
GLsizei wh = 700, ww = 1300; /* Initial window size - increased to fit all buttons */
GLfloat size = 5.0;         /* Point/Eraser size */
int draw_mode = 0;          /* Drawing mode */
int rx, ry;                 /* Cursor position */

GLfloat r = 0.0, g = 0.0, b = 0.0; /* Drawing color */
int fill = 0;                      /* Fill flag */
int font_type = 0;                 /* Font type: 0=9x15, 1=TIMES, 2=HELVETICA */
int rubberband = 0;                /* Rubberband mode flag */
int start_x, start_y;              /* Rubberband start point */
int rubber_x, rubber_y;            /* Rubberband current position */
int polygon_count = 0;             /* Polygon point count */
int polygon_points[100][2];        /* Polygon points array */
int eraser_mode = 0;               /* Eraser mode */
int spray_mode = 0;                /* Spray mode */
int polygon_mode = 0;              /* Polygon drawing mode */
int fill_bucket_mode = 0;          /* Fill bucket mode */
int eyedropper_mode = 0;           /* Eyedropper mode */
int brush_drawing = 0;             /* Brush continuous drawing flag */
int select_mode = 0;               /* Select mode */
int selecting = 0;                 /* Currently selecting */
int select_x1, select_y1, select_x2, select_y2;  /* Selection area */

/* Drawing history structure */
#define MAX_HISTORY 100
typedef struct {
    int type;
    int x1, y1, x2, y2;
    float r, g, b;
    float size;
    int filled;
    int points[100][2];
    int point_count;
} DrawingItem;

DrawingItem history[MAX_HISTORY];
int history_count = 0;
int redo_count = 0;  /* Redo stack count */

/* Stack for flood fill */
typedef struct {
    int x, y;
} Point;

#define STACK_SIZE 500000
Point fillStack[STACK_SIZE];
int stackTop = -1;

/* Draw square */
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

/* Spray effect */
void drawSpray(int x, int y) {
    y = wh - y;
    glColor3f(r, g, b);
    glPointSize(1.0);
    
    /* Save spray points for history */
    if (history_count < MAX_HISTORY) {
        history[history_count].type = SPRAY;
        history[history_count].r = r;
        history[history_count].g = g;
        history[history_count].b = b;
        history[history_count].point_count = 0;
        
        glBegin(GL_POINTS);
        for (int i = 0; i < 30; i++) {
            int dx = (rand() % (int)(size * 4)) - size * 2;
            int dy = (rand() % (int)(size * 4)) - size * 2;
            if (dx*dx + dy*dy <= size*size*4) {
                glVertex2i(x + dx, y + dy);
                if (history[history_count].point_count < 100) {
                    history[history_count].points[history[history_count].point_count][0] = x + dx;
                    history[history_count].points[history[history_count].point_count][1] = y + dy;
                    history[history_count].point_count++;
                }
            }
        }
        glEnd();
        
        if (history[history_count].point_count > 0) {
            history_count++;
            redo_count = 0;
        }
    } else {
        glBegin(GL_POINTS);
        for (int i = 0; i < 30; i++) {
            int dx = (rand() % (int)(size * 4)) - size * 2;
            int dy = (rand() % (int)(size * 4)) - size * 2;
            if (dx*dx + dy*dy <= size*size*4) {
                glVertex2i(x + dx, y + dy);
            }
        }
        glEnd();
    }
    
    glPointSize(size);
}

/* Eraser */
void drawEraser(int x, int y) {
    y = wh - y;
    glColor3f(0.8, 0.8, 0.8);  /* Background color */
    glBegin(GL_POLYGON);
    glVertex2f(x + size, y + size);
    glVertex2f(x - size, y + size);
    glVertex2f(x - size, y - size);
    glVertex2f(x + size, y - size);
    glEnd();
}

/* Rubberband rectangle */
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

/* Rubberband circle */
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

/* Draw polygon */
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
    
    /* Save to history */
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
        redo_count = 0;  /* Clear redo stack */
    }
    
    polygon_count = 0;
}

/* Delete last polygon */
void deleteLastPolygon(void) {
    /* Find and delete last polygon in history */
    for (int i = history_count - 1; i >= 0; i--) {
        if (history[i].type == POLYGON) {
            /* Shift all items after this one */
            for (int j = i; j < history_count - 1; j++) {
                history[j] = history[j + 1];
            }
            history_count--;
            redo_count = 0;
            redrawHistory();
            return;
        }
    }
}

/* Color picker */
void colorPicker(int x, int y) {
    /* Simple color picker: click position determines RGB values */
    r = (float)x / ww;
    g = (float)(wh - y) / wh;
    b = 1.0 - (float)(x + y) / (ww + wh);
    
    /* Ensure values are between 0-1 */
    if (r < 0.0) r = 0.0;
    if (r > 1.0) r = 1.0;
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    if (b < 0.0) b = 0.0;
    if (b > 1.0) b = 1.0;
    
    /* Update color preview immediately */
    display();
}

/* Mouse motion handler (rubberband effect) */
void motion(int x, int y) {
    if (rubberband) {
        /* Clear previous rubberband */
        if (draw_mode == RECTANGLE) {
            drawRubberRectangle(start_x, start_y, rubber_x, rubber_y);
        } else if (draw_mode == CIRCLE) {
            drawRubberCircle(start_x, start_y, rubber_x, rubber_y);
        }
        
        /* Draw new rubberband */
        if (draw_mode == RECTANGLE) {
            drawRubberRectangle(start_x, start_y, x, y);
        } else if (draw_mode == CIRCLE) {
            drawRubberCircle(start_x, start_y, x, y);
        }
        
        rubber_x = x;
        rubber_y = y;
    } else if (selecting && draw_mode == SELECT) {
        /* Draw selection rectangle */
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        glVertex2i(select_x1, wh - select_y1);
        glVertex2i(select_x1, wh - select_y2);
        glVertex2i(select_x2, wh - select_y2);
        glVertex2i(select_x2, wh - select_y1);
        glEnd();
        glLogicOp(GL_COPY);
        glDisable(GL_COLOR_LOGIC_OP);
        
        select_x2 = x;
        select_y2 = y;
        
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        glVertex2i(select_x1, wh - select_y1);
        glVertex2i(select_x1, wh - select_y2);
        glVertex2i(select_x2, wh - select_y2);
        glVertex2i(select_x2, wh - select_y1);
        glEnd();
        glLogicOp(GL_COPY);
        glDisable(GL_COLOR_LOGIC_OP);
        glFlush();
    } else if (brush_drawing && draw_mode == DRAW_POINTS) {
        /* Continuous brush drawing */
        glColor3f(r, g, b);
        int y_inv = wh - y;
        glBegin(GL_POLYGON);
        glVertex2f(x + size, y_inv + size);
        glVertex2f(x - size, y_inv + size);
        glVertex2f(x - size, y_inv - size);
        glVertex2f(x + size, y_inv - size);
        glEnd();
        glFlush();
    } else if (spray_mode) {
        drawSpray(x, y);
        glFlush();
    } else if (eraser_mode) {
        drawEraser(x, y);
        glFlush();
    }
}

/* Mouse event handler */
void mouse(int btn, int state, int x, int y) {
    static int count;
    int where;
    static int xp[2], yp[2];
    
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        where = pick(x, y);
        
        if (where != 0) {
            /* Only reset counter when switching modes */
            if (draw_mode != where) {
                count = 0;
                draw_mode = where;
                polygon_count = 0;  /* Reset polygon point count */
                
                /* Set special modes */
                spray_mode = (where == SPRAY);
                eraser_mode = (where == ERASER);
                polygon_mode = (where == POLYGON);
                fill_bucket_mode = (where == FILL_BUCKET);
                eyedropper_mode = (where == EYEDROPPER);
                select_mode = (where == SELECT);
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
                        
                        /* Save to history */
                        if (history_count < MAX_HISTORY) {
                            history[history_count].type = LINE;
                            history[history_count].x1 = xp[0];
                            history[history_count].y1 = yp[0];
                            history[history_count].x2 = x;
                            history[history_count].y2 = y;
                            history[history_count].r = r;
                            history[history_count].g = g;
                            history[history_count].b = b;
                            history_count++;
                            redo_count = 0;
                        }
                        
                        count = 0;  /* Reset counter but stay in line mode */
                    }
                    break;
                    
                case (RECTANGLE):
                    /* Press: start rubberband */
                    if (!rubberband) {
                        start_x = x;
                        start_y = y;
                        rubber_x = x;
                        rubber_y = y;
                        rubberband = 1;
                    }
                    break;
                    
                case (CIRCLE):
                    /* Press: start rubberband */
                    if (!rubberband) {
                        start_x = x;
                        start_y = y;
                        rubber_x = x;
                        rubber_y = y;
                        rubberband = 1;
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
                            glBegin(GL_LINE_LOOP);  /* Always draw outline */
                            glVertex2i(xp[0], wh - yp[0]);
                            glVertex2i(xp[1], wh - yp[1]);
                            glVertex2i(x, wh - y);
                            glEnd();
                            glFlush();
                            
                            /* Save to history */
                            if (history_count < MAX_HISTORY) {
                                history[history_count].type = TRIANGLE;
                                history[history_count].x1 = xp[0];
                                history[history_count].y1 = yp[0];
                                history[history_count].x2 = xp[1];
                                history[history_count].y2 = yp[1];
                                history[history_count].points[0][0] = x;
                                history[history_count].points[0][1] = y;
                                history[history_count].r = r;
                                history[history_count].g = g;
                                history[history_count].b = b;
                                history[history_count].filled = 0;  /* Always outline */
                                history_count++;
                                redo_count = 0;
                            }
                            
                            count = 0;  /* Reset counter but stay in triangle mode */
                    }
                    break;
                    
                case (DRAW_POINTS):
                    /* Start brush drawing */
                    brush_drawing = 1;
                    glColor3f(r, g, b);
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
                        
                        /* Draw point */
                        glColor3f(r, g, b);
                        glPointSize(3.0);
                        glBegin(GL_POINTS);
                        glVertex2i(x, wh - y);
                        glEnd();
                        glFlush();
                        
                        /* If enough points, draw line segment */
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
                    
                    /* Save eraser stroke to history */
                    if (history_count < MAX_HISTORY) {
                        history[history_count].type = ERASER;
                        history[history_count].x1 = x;
                        history[history_count].y1 = wh - y;
                        history[history_count].size = size;
                        history_count++;
                        redo_count = 0;
                    }
                    
                    glFlush();
                    break;
                    
                case (FILL_BUCKET):
                    /* Flood fill at clicked position */
                    if (x >= 0 && x < ww && y >= 0 && y < wh - ww / 13) {
                        GLubyte pixel[3];
                        glReadPixels(x, wh - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
                        float old_r = pixel[0] / 255.0f;
                        float old_g = pixel[1] / 255.0f;
                        float old_b = pixel[2] / 255.0f;
                        
                        /* Check if different color */
                        float tolerance = 0.02f;
                        if (fabs(old_r - r) > tolerance || fabs(old_g - g) > tolerance || fabs(old_b - b) > tolerance) {
                            scanlineFill(x, wh - y, old_r, old_g, old_b);
                        }
                    }
                    break;
                    
                case (EYEDROPPER):
                    /* Pick color from clicked position */
                    if (x >= 0 && x < ww && y >= 0 && y < wh) {
                        GLubyte pixel[3];
                        glReadPixels(x, wh - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
                        r = pixel[0] / 255.0f;
                        g = pixel[1] / 255.0f;
                        b = pixel[2] / 255.0f;
                        display();  /* Update color preview */
                    }
                    break;
                    
                case (SELECT):
                    /* Start selection */
                    selecting = 1;
                    select_x1 = x;
                    select_y1 = y;
                    select_x2 = x;
                    select_y2 = y;
                    break;
            }
        }
    } else if (btn == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        /* Stop selection */
        if (selecting && draw_mode == SELECT) {
            selecting = 0;
            /* Clear selection rectangle */
            glEnable(GL_COLOR_LOGIC_OP);
            glLogicOp(GL_XOR);
            glColor3f(1.0, 1.0, 1.0);
            glBegin(GL_LINE_LOOP);
            glVertex2i(select_x1, wh - select_y1);
            glVertex2i(select_x1, wh - select_y2);
            glVertex2i(select_x2, wh - select_y2);
            glVertex2i(select_x2, wh - select_y1);
            glEnd();
            glLogicOp(GL_COPY);
            glDisable(GL_COLOR_LOGIC_OP);
            glFlush();
        }
        
        /* Stop brush drawing and save to history */
        if (brush_drawing && draw_mode == DRAW_POINTS) {
            brush_drawing = 0;
            /* Save brush stroke to history (simplified) */
            if (history_count < MAX_HISTORY) {
                history[history_count].type = DRAW_POINTS;
                history[history_count].x1 = x;
                history[history_count].y1 = wh - y;
                history[history_count].r = r;
                history[history_count].g = g;
                history[history_count].b = b;
                history[history_count].size = size;
                history_count++;
                redo_count = 0;
            }
        }
        
        /* Release: complete rubberband drawing */
        if (rubberband && (draw_mode == RECTANGLE || draw_mode == CIRCLE)) {
            /* Clear final rubberband */
            if (draw_mode == RECTANGLE) {
                drawRubberRectangle(start_x, start_y, rubber_x, rubber_y);
            } else if (draw_mode == CIRCLE) {
                drawRubberCircle(start_x, start_y, rubber_x, rubber_y);
            }
            
            /* Draw final shape */
            if (draw_mode == RECTANGLE) {
                glColor3f(r, g, b);
                glBegin(GL_LINE_LOOP);  /* Always draw outline */
                glVertex2i(x, wh - y);
                glVertex2i(x, wh - start_y);
                glVertex2i(start_x, wh - start_y);
                glVertex2i(start_x, wh - y);
                glEnd();
                glFlush();
                
                /* Save to history */
                if (history_count < MAX_HISTORY) {
                    history[history_count].type = RECTANGLE;
                    history[history_count].x1 = start_x;
                    history[history_count].y1 = start_y;
                    history[history_count].x2 = x;
                    history[history_count].y2 = y;
                    history[history_count].r = r;
                    history[history_count].g = g;
                    history[history_count].b = b;
                    history[history_count].filled = 0;  /* Always outline */
                    history_count++;
                    redo_count = 0;
                }
            } else if (draw_mode == CIRCLE) {
                int radius = (int)sqrt((x-start_x)*(x-start_x) + 
                                      (y-start_y)*(y-start_y));
                glColor3f(r, g, b);
                glBegin(GL_LINE_LOOP);  /* Always draw outline */
                for (int i = 0; i < 100; i++) {
                    float angle = 2.0 * M_PI * i / 100.0;
                    glVertex2f(start_x + radius * cos(angle), 
                              wh - start_y + radius * sin(angle));
                }
                glEnd();
                glFlush();
                
                /* Save to history */
                if (history_count < MAX_HISTORY) {
                    history[history_count].type = CIRCLE;
                    history[history_count].x1 = start_x;
                    history[history_count].y1 = start_y;
                    history[history_count].x2 = x;
                    history[history_count].y2 = y;
                    history[history_count].r = r;
                    history[history_count].g = g;
                    history[history_count].b = b;
                    history[history_count].filled = 0;  /* Always outline */
                    history_count++;
                    redo_count = 0;
                }
            }
            
            rubberband = 0;
        }
    } else if (btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        /* Right click to complete polygon */
        if (draw_mode == POLYGON && polygon_count >= 3) {
            drawPolygon();
            draw_mode = 0;
            polygon_mode = 0;
        }
        /* Right click for color selection */
        else if (draw_mode == 0) {
            colorPicker(x, y);
        }
    }
}

/* Select toolbar item */
int pick(int x, int y) {
    y = wh - y;
    int btn_width = ww / 13;  /* 13 buttons total (12 tools + 1 color preview) */
    if (y < wh - btn_width)
        return 0;
    else if (x < btn_width)
        return LINE;
    else if (x < 2 * btn_width)
        return RECTANGLE;
    else if (x < 3 * btn_width)
        return TRIANGLE;
    else if (x < 4 * btn_width)
        return DRAW_POINTS;
    else if (x < 5 * btn_width)
        return DRAW_TEXT;
    else if (x < 6 * btn_width)
        return POLYGON;
    else if (x < 7 * btn_width)
        return CIRCLE;
    else if (x < 8 * btn_width)
        return ERASER;
    else if (x < 9 * btn_width)
        return SPRAY;
    else if (x < 10 * btn_width)
        return FILL_BUCKET;
    else if (x < 11 * btn_width)
        return EYEDROPPER;
    else if (x < 12 * btn_width)
        return SELECT;
    else
        return 0;  /* Last button is color preview, not clickable */
}

/* Save drawing */
void saveDrawing(const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (fp) {
        /* Save history */
        fwrite(&history_count, sizeof(int), 1, fp);
        fwrite(history, sizeof(DrawingItem), history_count, fp);
        fclose(fp);
        printf("Drawing saved to %s\n", filename);
    }
}

/* Load drawing */
void loadDrawing(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp) {
        /* Read history */
        fread(&history_count, sizeof(int), 1, fp);
        fread(history, sizeof(DrawingItem), history_count, fp);
        fclose(fp);
        
        /* Reset redo stack */
        redo_count = 0;
        
        /* Redraw all items */
        redrawHistory();
        printf("Drawing loaded from %s\n", filename);
    }
}

/* Scanline flood fill - more efficient than stack-based */
void scanlineFill(int x, int y, float old_r, float old_g, float old_b) {
    stackTop = -1;
    float tolerance = 0.02f;
    int maxY = wh - ww / 13;  /* Don't fill toolbar area */
    
    /* Create visited array to prevent infinite loops */
    static unsigned char *visited = NULL;
    if (visited == NULL) {
        visited = (unsigned char*)calloc(ww * wh, sizeof(unsigned char));
    }
    memset(visited, 0, ww * wh * sizeof(unsigned char));
    
    /* Push starting point */
    if (stackTop < STACK_SIZE - 1) {
        fillStack[++stackTop].x = x;
        fillStack[stackTop].y = y;
    }
    
    glColor3f(r, g, b);
    glBegin(GL_POINTS);
    
    while (stackTop >= 0) {
        Point p = fillStack[stackTop--];
        int px = p.x;
        int py = p.y;
        
        if (px < 0 || px >= ww || py < 0 || py >= maxY) continue;
        
        /* Check if already visited */
        int idx = py * ww + px;
        if (visited[idx]) continue;
        visited[idx] = 1;
        
        GLubyte pixel[3];
        glReadPixels(px, py, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
        float curr_r = pixel[0] / 255.0f;
        float curr_g = pixel[1] / 255.0f;
        float curr_b = pixel[2] / 255.0f;
        
        /* Check if color matches */
        if (fabs(curr_r - old_r) > tolerance || 
            fabs(curr_g - old_g) > tolerance || 
            fabs(curr_b - old_b) > tolerance) {
            continue;
        }
        
        /* Paint pixel */
        glVertex2i(px, py);
        
        /* Add 4-way neighbors to stack */
        if (stackTop < STACK_SIZE - 4) {
            if (px + 1 < ww) {
                fillStack[++stackTop].x = px + 1;
                fillStack[stackTop].y = py;
            }
            if (px - 1 >= 0) {
                fillStack[++stackTop].x = px - 1;
                fillStack[stackTop].y = py;
            }
            if (py + 1 < maxY) {
                fillStack[++stackTop].x = px;
                fillStack[stackTop].y = py + 1;
            }
            if (py - 1 >= 0) {
                fillStack[++stackTop].x = px;
                fillStack[stackTop].y = py - 1;
            }
        }
    }
    
    glEnd();
    glFlush();
}

/* Simple PPM to JPG conversion using external command (if available) */
/* Or save as PPM which can be converted later */
void exportImageJPG(const char* filename) {
    int width = ww;
    int height = wh - ww / 12;  /* Exclude toolbar */
    
    unsigned char* pixels = (unsigned char*)malloc(3 * width * height);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    
    /* Save as PPM first (simple format) */
    char ppmFilename[256];
    sprintf(ppmFilename, "%s.ppm", filename);
    
    FILE* fp = fopen(ppmFilename, "wb");
    if (fp) {
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
        
        /* PPM stores top-to-bottom, OpenGL reads bottom-to-top */
        for (int y = height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                fputc(pixels[idx], fp);     /* R */
                fputc(pixels[idx + 1], fp); /* G */
                fputc(pixels[idx + 2], fp); /* B */
            }
        }
        
        fclose(fp);
        free(pixels);
        printf("Image exported to %s (PPM format)\n", ppmFilename);
        printf("Note: You can convert PPM to JPG using external tools\n");
    } else {
        free(pixels);
    }
}

/* Redraw all history items */
void redrawHistory(void) {
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    display();
    
    for (int i = 0; i < history_count; i++) {
        glColor3f(history[i].r, history[i].g, history[i].b);
        
        switch (history[i].type) {
            case LINE:
                glBegin(GL_LINES);
                glVertex2i(history[i].x1, wh - history[i].y1);
                glVertex2i(history[i].x2, wh - history[i].y2);
                glEnd();
                break;
                
            case RECTANGLE:
                glBegin(GL_LINE_LOOP);
                glVertex2i(history[i].x2, wh - history[i].y2);
                glVertex2i(history[i].x2, wh - history[i].y1);
                glVertex2i(history[i].x1, wh - history[i].y1);
                glVertex2i(history[i].x1, wh - history[i].y2);
                glEnd();
                break;
                
            case TRIANGLE:
                glBegin(GL_LINE_LOOP);
                glVertex2i(history[i].x1, wh - history[i].y1);
                glVertex2i(history[i].x2, wh - history[i].y2);
                glVertex2i(history[i].points[0][0], wh - history[i].points[0][1]);
                glEnd();
                break;
                
            case CIRCLE:
                {
                    int radius = (int)sqrt((history[i].x2 - history[i].x1) * (history[i].x2 - history[i].x1) + 
                                          (history[i].y2 - history[i].y1) * (history[i].y2 - history[i].y1));
                    glBegin(GL_LINE_LOOP);
                    for (int j = 0; j < 100; j++) {
                        float angle = 2.0 * M_PI * j / 100.0;
                        glVertex2f(history[i].x1 + radius * cos(angle), 
                                  wh - history[i].y1 + radius * sin(angle));
                    }
                    glEnd();
                }
                break;
                
            case DRAW_POINTS:
                {
                    float s = history[i].size;
                    glBegin(GL_POLYGON);
                    glVertex2f(history[i].x1 + s, history[i].y1 + s);
                    glVertex2f(history[i].x1 - s, history[i].y1 + s);
                    glVertex2f(history[i].x1 - s, history[i].y1 - s);
                    glVertex2f(history[i].x1 + s, history[i].y1 - s);
                    glEnd();
                }
                break;
                
            case SPRAY:
                glPointSize(1.0);
                glBegin(GL_POINTS);
                for (int j = 0; j < history[i].point_count; j++) {
                    glVertex2i(history[i].points[j][0], history[i].points[j][1]);
                }
                glEnd();
                glPointSize(size);
                break;
                
            case ERASER:
                {
                    float s = history[i].size;
                    glColor3f(0.8, 0.8, 0.8);  /* Background color */
                    glBegin(GL_POLYGON);
                    glVertex2f(history[i].x1 + s, history[i].y1 + s);
                    glVertex2f(history[i].x1 - s, history[i].y1 + s);
                    glVertex2f(history[i].x1 - s, history[i].y1 - s);
                    glVertex2f(history[i].x1 + s, history[i].y1 - s);
                    glEnd();
                }
                break;
                
            case POLYGON:
                if (history[i].filled)
                    glBegin(GL_POLYGON);
                else
                    glBegin(GL_LINE_LOOP);
                for (int j = 0; j < history[i].point_count; j++) {
                    glVertex2i(history[i].points[j][0], 
                              wh - history[i].points[j][1]);
                }
                glEnd();
                break;
        }
    }
    glFlush();
}

/* Undo */
void undo(void) {
    if (history_count > 0) {
        redo_count++;
        history_count--;
        redrawHistory();
    }
}

/* Redo */
void redo(void) {
    if (redo_count > 0) {
        redo_count--;
        history_count++;
        redrawHistory();
    }
}

/* Window resize */
void myReshape(GLsizei w, GLsizei h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)w, 0.0, (GLdouble)h, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, w, h);
    
    /* Clear entire window including old toolbar area */
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    /* Update window size */
    ww = w;
    wh = h;
    
    /* Redraw toolbar at new position */
    display();
    glFlush();
}

/* Initialization */
void myinit(void) {
    glViewport(0, 0, ww, wh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)ww, 0.0, (GLdouble)wh, -1.0, 1.0);
    glClearColor(0.8, 0.8, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    srand(time(NULL));  /* Initialize random number generator */
}

/* Toolbar box drawing */
void screen_box(int x, int y, int s) {
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + s, y);
    glVertex2i(x + s, y + s);
    glVertex2i(x, y + s);
    glEnd();
}

/* Right click menu */
void right_menu(int id) {
    if (id == 1)
        exit(0);
    else if (id == 2) {
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
        glFlush();
        history_count = 0;  /* Clear history */
        redo_count = 0;     /* Clear redo stack */
    } else if (id == 3) {
        saveDrawing("drawing.bin");
    } else if (id == 4) {
        loadDrawing("drawing.bin");
    } else if (id == 5) {
        undo();
    } else if (id == 6) {
        redo();
    } else if (id == 7) {
        deleteLastPolygon();
    }
}

/* Middle button menu */
void middle_menu(int id) {
    /* Empty implementation, can be extended */
}

/* Color menu */
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
    
    /* Update color preview immediately */
    display();
}

/* Pixel size menu */
void pixel_menu(int id) {
    if (id == 1 && size < 50.0) size *= 1.5;
    else if (id == 2 && size > 1.0) size /= 1.5;
}

/* Fill menu */
void fill_menu(int id) {
    fill = (id == 1);
    /* Update display to show fill mode change */
    display();
}

/* Font menu */
void font_menu(int id) {
    font_type = id - 1;
}

/* Tool menu */
void tool_menu(int id) {
    draw_mode = id;
}

/* Keyboard event handler */
void key(unsigned char k, int xx, int yy) {
    if (draw_mode == DRAW_TEXT) {
        glColor3f(r, g, b);
        glRasterPos2i(rx, ry);
        
        /* Draw text based on selected font */
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
    } else if (k == 'r' || k == 'R') {
        redo();
    } else if (k == 'd' || k == 'D') {
        deleteLastPolygon();
    } else if (k == 'c' || k == 'C') {
        glClearColor(0.8, 0.8, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        display();
        glFlush();
        history_count = 0;
        redo_count = 0;
    }
}

/* Display function - Draw toolbar */
void display(void) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    /* Draw toolbar background */
    int btn_width = ww / 13;  /* 13 buttons total (12 tools + 1 color preview) */
    int btn_height = ww / 13;
    glColor3f(0.6, 0.6, 0.6);
    glBegin(GL_QUADS);
    glVertex2i(0, wh - btn_height);
    glVertex2i(ww, wh - btn_height);
    glVertex2i(ww, wh);
    glVertex2i(0, wh);
    glEnd();
    
    /* Draw tool buttons */
    int start_y = wh - btn_height;
    
    /* Line button */
    glColor3f(1.0, 1.0, 1.0);
    screen_box(0, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINES);
    glVertex2i(btn_width/4, start_y + btn_height/2);
    glVertex2i(3*btn_width/4, start_y + btn_height/2);
    glEnd();
    
    /* Rectangle button */
    glColor3f(1.0, 0.0, 0.0);
    screen_box(btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(btn_width + btn_width/4, start_y + btn_height/4);
    glVertex2i(btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(btn_width + 3*btn_width/4, start_y + btn_height/4);
    glEnd();
    
    /* Triangle button */
    glColor3f(0.0, 1.0, 0.0);
    screen_box(2*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex2i(2*btn_width + btn_width/2, start_y + btn_height/4);
    glVertex2i(2*btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(2*btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glEnd();
    
    /* Point button */
    glColor3f(0.0, 0.0, 1.0);
    screen_box(3*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex2i(3*btn_width + btn_width/2, start_y + btn_height/2);
    glEnd();
    
    /* Text button */
    glColor3f(1.0, 1.0, 0.0);
    screen_box(4*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2i(4*btn_width + 5, start_y + 5);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'T');
    
    /* Polygon button */
    glColor3f(0.5, 0.5, 1.0);
    screen_box(5*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(5*btn_width + btn_width/2, start_y + btn_height/4);
    glVertex2i(5*btn_width + btn_width/4, start_y + btn_height/2);
    glVertex2i(5*btn_width + btn_width/2, start_y + 3*btn_height/4);
    glVertex2i(5*btn_width + 3*btn_width/4, start_y + btn_height/2);
    glEnd();
    
    /* Circle button */
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
    
    /* Eraser button */
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
    
    /* Spray button */
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
    
    /* Fill bucket button */
    glColor3f(1.0, 0.8, 0.6);
    screen_box(9*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_POLYGON);
    glVertex2i(9*btn_width + btn_width/2, start_y + btn_height/4);
    glVertex2i(9*btn_width + btn_width/4, start_y + 2*btn_height/3);
    glVertex2i(9*btn_width + 3*btn_width/4, start_y + 2*btn_height/3);
    glEnd();
    glBegin(GL_LINES);
    glVertex2i(9*btn_width + btn_width/2 - 5, start_y + btn_height/4);
    glVertex2i(9*btn_width + btn_width/2 - 5, start_y + 10);
    glEnd();
    
    /* Eyedropper button */
    glColor3f(0.9, 0.9, 0.9);
    screen_box(10*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINES);
    glVertex2i(10*btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(10*btn_width + 3*btn_width/4, start_y + btn_height/4);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex2i(10*btn_width + 3*btn_width/4 - 3, start_y + btn_height/4 - 3);
    glVertex2i(10*btn_width + 3*btn_width/4 + 3, start_y + btn_height/4 - 3);
    glVertex2i(10*btn_width + 3*btn_width/4 + 3, start_y + btn_height/4 + 3);
    glVertex2i(10*btn_width + 3*btn_width/4 - 3, start_y + btn_height/4 + 3);
    glEnd();
    
    /* Select button */
    glColor3f(0.7, 0.7, 1.0);
    screen_box(11*btn_width, start_y, btn_width);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(11*btn_width + btn_width/4, start_y + btn_height/4);
    glVertex2i(11*btn_width + btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(11*btn_width + 3*btn_width/4, start_y + 3*btn_height/4);
    glVertex2i(11*btn_width + 3*btn_width/4, start_y + btn_height/4);
    glEnd();
    /* Draw selection cursor */
    glBegin(GL_LINES);
    glVertex2i(11*btn_width + btn_width/3, start_y + btn_height/3);
    glVertex2i(11*btn_width + 2*btn_width/3, start_y + 2*btn_height/3);
    glVertex2i(11*btn_width + 2*btn_width/3, start_y + btn_height/3);
    glVertex2i(11*btn_width + btn_width/3, start_y + 2*btn_height/3);
    glEnd();
    
    /* Color preview - at the last position (rightmost), separate from Select */
    glColor3f(r, g, b);
    screen_box(12*btn_width, start_y, btn_width);
    
    /* Clear status area and display status */
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(500, 0);
    glVertex2i(500, 25);
    glVertex2i(0, 25);
    glEnd();
    
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2i(10, 10);
    char status[100];
    sprintf(status, "Mode: %d Color: %.1f,%.1f,%.1f Size: %.1f", 
            draw_mode, r, g, b, size);
    for (int i = 0; status[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, status[i]);
    }
    
    glFlush();
    glPopAttrib();
}

/* Main function */
int main(int argc, char **argv) {
    int c_menu, p_menu, f_menu, fo_menu, t_menu;
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(ww, wh);
    glutCreateWindow("Super Paint - Midterm Project");
    
    /* Create color menu */
    c_menu = glutCreateMenu(color_menu);
    glutAddMenuEntry("Red", 1);
    glutAddMenuEntry("Green", 2);
    glutAddMenuEntry("Blue", 3);
    glutAddMenuEntry("Cyan", 4);
    glutAddMenuEntry("Magenta", 5);
    glutAddMenuEntry("Yellow", 6);
    glutAddMenuEntry("White", 7);
    glutAddMenuEntry("Black", 8);
    glutAddMenuEntry("Gray", 9);
    glutAddMenuEntry("Orange", 10);
    
    /* Create pixel size menu */
    p_menu = glutCreateMenu(pixel_menu);
    glutAddMenuEntry("Increase Size", 1);
    glutAddMenuEntry("Decrease Size", 2);
    
    /* Create fill menu */
    f_menu = glutCreateMenu(fill_menu);
    glutAddMenuEntry("Fill On", 1);
    glutAddMenuEntry("Fill Off", 2);
    
    /* Create font menu */
    fo_menu = glutCreateMenu(font_menu);
    glutAddMenuEntry("9x15 Font", 1);
    glutAddMenuEntry("Times Roman", 2);
    glutAddMenuEntry("Helvetica", 3);
    
    /* Create right click menu */
    glutCreateMenu(right_menu);
    glutAddMenuEntry("Exit", 1);
    glutAddMenuEntry("Clear", 2);
    glutAddMenuEntry("Save", 3);
    glutAddMenuEntry("Load", 4);
    glutAddMenuEntry("Undo", 5);
    glutAddMenuEntry("Redo", 6);
    glutAddMenuEntry("Delete Polygon", 7);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    /* Create middle button menu */
    glutCreateMenu(middle_menu);
    glutAddSubMenu("Color", c_menu);
    glutAddSubMenu("Size", p_menu);
    glutAddSubMenu("Fill", f_menu);
    glutAddSubMenu("Font", fo_menu);
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