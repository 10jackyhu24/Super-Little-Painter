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
#define BEZIER_SURFACE 13

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
void initBezierSurface(void);
void displayBezierSurface(void);
void displayControlWindow(void);
void reshapeBezierSurface(int, int);
void reshapeControlWindow(int, int);
void mouseBezierSurface(int, int, int, int);
void motionBezierSurface(int, int);
void mouseControlWindow(int, int, int, int);
void motionControlWindow(int, int);
void keyBezierSurface(unsigned char, int, int);
float bernstein(int, int, float);
void computeBezierSurface(float, float, float*);

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
int select_x1 = 0, select_y1 = 0, select_x2 = 0, select_y2 = 0;  /* Selection area */
int select_has_content = 0;        /* Whether selection has captured content */
unsigned char* selection_buffer = NULL;  /* Selection buffer */
int selection_width = 0, selection_height = 0;   /* Selection dimensions */
int selection_origin_x = 0, selection_origin_y = 0;  /* Original position */
int moving_selection = 0;          /* Currently moving selection */
int move_offset_x = 0, move_offset_y = 0;  /* Move offset from origin */

/* Bezier Surface Variables */
int bezier_surface_window = 0;     /* Bezier surface window ID */
int control_window = 0;             /* Control window ID */
int show_bezier = 0;                /* Show Bezier surface flag */
float controlPoints[4][4][3];       /* 4x4 control points for Bezier surface */
int selectedPoint = -1;             /* Currently selected control point */
int selectedI = 0, selectedJ = 0;   /* Selected point indices */
float rotateX = 30.0, rotateY = 45.0;  /* Rotation angles */
int lastMouseX = 0, lastMouseY = 0; /* Last mouse position */

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
    unsigned char* fill_data;  /* For fill bucket undo */
    int fill_width, fill_height, fill_x, fill_y;  /* Fill area info */
} DrawingItem;

DrawingItem history[MAX_HISTORY];
DrawingItem redo_stack[MAX_HISTORY];  /* Separate redo stack */
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
        history[history_count].fill_data = NULL;
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
    /* Handle moving selection */
    if (moving_selection && select_has_content) {
        int max_y = wh - ww / 13;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= ww) x = ww - 1;
        if (y >= max_y) y = max_y - 1;
        
        /* Clear old selection rectangle using XOR */
        int old_draw_x = selection_origin_x + move_offset_x;
        int old_draw_y = selection_origin_y + move_offset_y;
        
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glColor3f(1.0, 1.0, 1.0);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0xAAAA);
        glBegin(GL_LINE_LOOP);
        glVertex2i(old_draw_x, wh - old_draw_y);
        glVertex2i(old_draw_x + selection_width, wh - old_draw_y);
        glVertex2i(old_draw_x + selection_width, wh - old_draw_y - selection_height);
        glVertex2i(old_draw_x, wh - old_draw_y - selection_height);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
        
        /* Calculate new offset */
        move_offset_x = x - select_x1;
        move_offset_y = y - select_y1;
        
        /* Draw new selection rectangle */
        int new_draw_x = selection_origin_x + move_offset_x;
        int new_draw_y = selection_origin_y + move_offset_y;
        
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0xAAAA);
        glBegin(GL_LINE_LOOP);
        glVertex2i(new_draw_x, wh - new_draw_y);
        glVertex2i(new_draw_x + selection_width, wh - new_draw_y);
        glVertex2i(new_draw_x + selection_width, wh - new_draw_y - selection_height);
        glVertex2i(new_draw_x, wh - new_draw_y - selection_height);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
        glLogicOp(GL_COPY);
        glDisable(GL_COLOR_LOGIC_OP);
        glFlush();
        return;
    }
    
    /* Handle drawing selection rectangle */
    if (selecting && draw_mode == SELECT) {
        /* Clamp coordinates to valid range */
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        int max_y = wh - ww / 13;
        if (x >= ww) x = ww - 1;
        if (y >= max_y) y = max_y - 1;
        
        /* Clear old rectangle */
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        glVertex2i(select_x1, wh - select_y1);
        glVertex2i(select_x1, wh - select_y2);
        glVertex2i(select_x2, wh - select_y2);
        glVertex2i(select_x2, wh - select_y1);
        glEnd();
        
        /* Update coordinates */
        select_x2 = x;
        select_y2 = y;
        
        /* Draw new rectangle */
        glBegin(GL_LINE_LOOP);
        glVertex2i(select_x1, wh - select_y1);
        glVertex2i(select_x1, wh - select_y2);
        glVertex2i(select_x2, wh - select_y2);
        glVertex2i(select_x2, wh - select_y1);
        glEnd();
        glLogicOp(GL_COPY);
        glDisable(GL_COLOR_LOGIC_OP);
        glFlush();
        return;
    }
    
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
        
        /* Save points to history */
        if (history_count < MAX_HISTORY && history[history_count].point_count < 100) {
            history[history_count].points[history[history_count].point_count][0] = x;
            history[history_count].points[history[history_count].point_count][1] = y_inv;
            history[history_count].point_count++;
        }
    } else if (spray_mode) {
        drawSpray(x, y);
        /* Save spray points to history */
        if (history_count < MAX_HISTORY && history[history_count].point_count < 100) {
            for (int i = 0; i < 30 && history[history_count].point_count < 100; i++) {
                int dx = (rand() % (int)(size * 4)) - size * 2;
                int dy = (rand() % (int)(size * 4)) - size * 2;
                if (dx*dx + dy*dy <= size*size*4) {
                    history[history_count].points[history[history_count].point_count][0] = x + dx;
                    history[history_count].points[history[history_count].point_count][1] = wh - y + dy;
                    history[history_count].point_count++;
                }
            }
        }
        glFlush();
    } else if (eraser_mode) {
        drawEraser(x, y);
        /* Save eraser points to history */
        if (history_count < MAX_HISTORY && history[history_count].point_count < 100) {
            history[history_count].points[history[history_count].point_count][0] = x;
            history[history_count].points[history[history_count].point_count][1] = wh - y;
            history[history_count].point_count++;
        }
        glFlush();
    }
}

/* Mouse event handler */
void mouse(int btn, int state, int x, int y) {
    static int count;
    int where;
    static int xp[2], yp[2];
    static int spray_start = 0;
    static int eraser_start = 0;
    static int brush_start = 0;
    
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        /* Check if clicking Bezier Surface button */
        int bezier_btn_size = 80;
        int bezier_x = ww - bezier_btn_size - 10;
        int bezier_y = 10;
        
        if (x >= bezier_x && x <= bezier_x + bezier_btn_size &&
            y >= bezier_y && y <= bezier_y + bezier_btn_size) {
            /* Toggle Bezier surface display */
            if (!show_bezier) {
                /* Initialize and show Bezier surface windows */
                initBezierSurface();
                show_bezier = 1;
                
                /* Create Bezier surface window */
                glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
                bezier_surface_window = glutCreateWindow("Bezier Surface");
                glutPositionWindow(100, 100);
                glutReshapeWindow(600, 600);
                glEnable(GL_DEPTH_TEST);
                glClearColor(0.0, 0.0, 0.0, 1.0);
                glutDisplayFunc(displayBezierSurface);
                glutReshapeFunc(reshapeBezierSurface);
                glutMouseFunc(mouseBezierSurface);
                glutMotionFunc(motionBezierSurface);
                glutKeyboardFunc(keyBezierSurface);
                
                /* Create control window */
                glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
                control_window = glutCreateWindow("Bezier Surface Control");
                glutPositionWindow(720, 100);
                glutReshapeWindow(400, 400);
                glClearColor(0.9, 0.9, 0.9, 1.0);
                glutDisplayFunc(displayControlWindow);
                glutReshapeFunc(reshapeControlWindow);
                glutMouseFunc(mouseControlWindow);
                glutMotionFunc(motionControlWindow);
                glutKeyboardFunc(keyBezierSurface);
                
                /* Redraw main window to show button as active */
                glutSetWindow(1); /* Main window */
                display();
                glFlush();
            } else {
                /* Close Bezier surface windows */
                if (bezier_surface_window) {
                    glutDestroyWindow(bezier_surface_window);
                    bezier_surface_window = 0;
                }
                if (control_window) {
                    glutDestroyWindow(control_window);
                    control_window = 0;
                }
                show_bezier = 0;
                
                /* Redraw main window to show button as inactive */
                glutSetWindow(1); /* Main window */
                display();
                glFlush();
            }
            return;
        }
        
        where = pick(x, y);
        
        /* Handle selection tool */
        if (draw_mode == SELECT && where == 0) {
            /* Check if clicking inside existing selection to move it */
            if (select_has_content) {
                int min_x = selection_origin_x + move_offset_x;
                int max_x = min_x + selection_width;
                int min_y = selection_origin_y + move_offset_y;
                int max_y = min_y + selection_height;
                
                if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
                    /* Start moving the selection */
                    moving_selection = 1;
                    select_x1 = x;
                    select_y1 = y;
                    
                    /* Draw initial selection rectangle */
                    glEnable(GL_COLOR_LOGIC_OP);
                    glLogicOp(GL_XOR);
                    glColor3f(1.0, 1.0, 1.0);
                    glEnable(GL_LINE_STIPPLE);
                    glLineStipple(1, 0xAAAA);
                    glBegin(GL_LINE_LOOP);
                    glVertex2i(min_x, wh - min_y);
                    glVertex2i(max_x, wh - min_y);
                    glVertex2i(max_x, wh - max_y);
                    glVertex2i(min_x, wh - max_y);
                    glEnd();
                    glDisable(GL_LINE_STIPPLE);
                    glLogicOp(GL_COPY);
                    glDisable(GL_COLOR_LOGIC_OP);
                    glFlush();
                    
                    return;
                }
            }
            
            /* Start new selection - clear old one */
            if (selection_buffer != NULL) {
                free(selection_buffer);
                selection_buffer = NULL;
            }
            selection_width = 0;
            selection_height = 0;
            select_has_content = 0;
            move_offset_x = 0;
            move_offset_y = 0;
            
            selecting = 1;
            select_x1 = x;
            select_y1 = y;
            select_x2 = x;
            select_y2 = y;
            return;
        }
        
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
                            history[history_count].fill_data = NULL;
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
                                history[history_count].fill_data = NULL;
                                history_count++;
                                redo_count = 0;
                            }
                            
                            count = 0;  /* Reset counter but stay in triangle mode */
                    }
                    break;
                    
                case (DRAW_POINTS):
                    {
                        /* Start brush drawing */
                        brush_drawing = 1;
                        brush_start = 1;
                        glColor3f(r, g, b);
                        int y_inv = wh - y;
                        glBegin(GL_POLYGON);
                        glVertex2f(x + size, y_inv + size);
                        glVertex2f(x - size, y_inv + size);
                        glVertex2f(x - size, y_inv - size);
                        glVertex2f(x + size, y_inv - size);
                        glEnd();
                        glFlush();
                        
                        /* Start new brush stroke in history */
                        if (history_count < MAX_HISTORY) {
                            history[history_count].type = DRAW_POINTS;
                            history[history_count].r = r;
                            history[history_count].g = g;
                            history[history_count].b = b;
                            history[history_count].size = size;
                            history[history_count].point_count = 0;
                            history[history_count].fill_data = NULL;
                        }
                    }
                    break;
                    
                case (DRAW_TEXT):
                    rx = x;
                    ry = wh - y;
                    glRasterPos2i(rx, ry);
                    break;
                    
                case (POLYGON):
                    {
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
                    }
                    break;
                    
                case (SPRAY):
                    {
                        spray_start = 1;
                        /* Start new spray stroke in history */
                        if (history_count < MAX_HISTORY) {
                            history[history_count].type = SPRAY;
                            history[history_count].r = r;
                            history[history_count].g = g;
                            history[history_count].b = b;
                            history[history_count].point_count = 0;
                            history[history_count].fill_data = NULL;
                        }
                        drawSpray(x, y);
                        /* Save spray points */
                        if (history_count < MAX_HISTORY && history[history_count].point_count < 100) {
                            for (int i = 0; i < 30 && history[history_count].point_count < 100; i++) {
                                int dx = (rand() % (int)(size * 4)) - size * 2;
                                int dy = (rand() % (int)(size * 4)) - size * 2;
                                if (dx*dx + dy*dy <= size*size*4) {
                                    history[history_count].points[history[history_count].point_count][0] = x + dx;
                                    history[history_count].points[history[history_count].point_count][1] = wh - y + dy;
                                    history[history_count].point_count++;
                                }
                            }
                        }
                        glFlush();
                    }
                    break;
                    
                case (ERASER):
                    {
                        eraser_start = 1;
                        drawEraser(x, y);
                        
                        /* Start new eraser stroke in history */
                        if (history_count < MAX_HISTORY) {
                            history[history_count].type = ERASER;
                            history[history_count].size = size;
                            history[history_count].point_count = 0;
                            history[history_count].fill_data = NULL;
                        }
                        glFlush();
                    }
                    break;
                    
                case (FILL_BUCKET):
                    {
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
                                /* Save area before filling for undo */
                                int max_y = wh - ww / 13;
                                int area_size = ww * max_y * 3;
                                unsigned char* before_fill = (unsigned char*)malloc(area_size);
                                glReadPixels(0, 0, ww, max_y, GL_RGB, GL_UNSIGNED_BYTE, before_fill);
                                
                                /* Perform fill */
                                scanlineFill(x, wh - y, old_r, old_g, old_b);
                                
                                /* Save to history */
                                if (history_count < MAX_HISTORY) {
                                    history[history_count].type = FILL_BUCKET;
                                    history[history_count].x1 = x;
                                    history[history_count].y1 = wh - y;
                                    history[history_count].r = r;
                                    history[history_count].g = g;
                                    history[history_count].b = b;
                                    history[history_count].fill_data = before_fill;
                                    history[history_count].fill_width = ww;
                                    history[history_count].fill_height = max_y;
                                    history[history_count].fill_x = 0;
                                    history[history_count].fill_y = 0;
                                    history_count++;
                                    redo_count = 0;
                                } else {
                                    free(before_fill);
                                }
                            }
                        }
                    }
                    break;
                    
                case (EYEDROPPER):
                    {
                        /* Pick color from clicked position */
                        if (x >= 0 && x < ww && y >= 0 && y < wh) {
                            GLubyte pixel[3];
                            glReadPixels(x, wh - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
                            r = pixel[0] / 255.0f;
                            g = pixel[1] / 255.0f;
                            b = pixel[2] / 255.0f;
                            display();  /* Update color preview */
                        }
                    }
                    break;
                    
                case (SELECT):
                {
                    /* This case is now handled at the top of GLUT_DOWN */
                    /* Do nothing here */
                }
                break;
            }
        }
    } else if (btn == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        /* Handle moving selection completion */
        if (moving_selection && select_has_content) {
            moving_selection = 0;
            
            /* Clear the selection rectangle */
            int draw_x = selection_origin_x + move_offset_x;
            int draw_y = selection_origin_y + move_offset_y;
            
            glEnable(GL_COLOR_LOGIC_OP);
            glLogicOp(GL_XOR);
            glColor3f(1.0, 1.0, 1.0);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, 0xAAAA);
            glBegin(GL_LINE_LOOP);
            glVertex2i(draw_x, wh - draw_y);
            glVertex2i(draw_x + selection_width, wh - draw_y);
            glVertex2i(draw_x + selection_width, wh - draw_y - selection_height);
            glVertex2i(draw_x, wh - draw_y - selection_height);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            glLogicOp(GL_COPY);
            glDisable(GL_COLOR_LOGIC_OP);
            
            /* Paste selection at new location */
            if (selection_buffer != NULL && history_count < MAX_HISTORY) {
                int buffer_size = selection_width * selection_height * 3;
                unsigned char* copy_buffer = (unsigned char*)malloc(buffer_size);
                if (copy_buffer != NULL) {
                    memcpy(copy_buffer, selection_buffer, buffer_size);
                    
                    int new_x = selection_origin_x + move_offset_x;
                    int new_y = selection_origin_y + move_offset_y;
                    
                    /* Draw the selection at new position */
                    int draw_y_gl = wh - new_y - selection_height;
                    glRasterPos2i(new_x, draw_y_gl);
                    glDrawPixels(selection_width, selection_height, 
                                GL_RGB, GL_UNSIGNED_BYTE, selection_buffer);
                    glFlush();
                    
                    history[history_count].type = SELECT;
                    history[history_count].x1 = new_x;
                    history[history_count].y1 = new_y;
                    history[history_count].fill_data = copy_buffer;
                    history[history_count].fill_width = selection_width;
                    history[history_count].fill_height = selection_height;
                    history_count++;
                    redo_count = 0;
                    
                    /* Update origin for next move */
                    selection_origin_x = new_x;
                    selection_origin_y = new_y;
                    move_offset_x = 0;
                    move_offset_y = 0;
                }
            }
            return;
        }
        
        /* Handle selection completion */
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
            
            /* Calculate selection bounds */
            int min_x = select_x1 < select_x2 ? select_x1 : select_x2;
            int max_x = select_x1 > select_x2 ? select_x1 : select_x2;
            int min_y = select_y1 < select_y2 ? select_y1 : select_y2;
            int max_y = select_y1 > select_y2 ? select_y1 : select_y2;
            
            int max_screen_y = wh - ww / 13;
            if (min_x < 0) min_x = 0;
            if (min_y < 0) min_y = 0;
            if (max_x > ww) max_x = ww;
            if (max_y > max_screen_y) max_y = max_screen_y;
            
            selection_width = max_x - min_x;
            selection_height = max_y - min_y;
            
            /* Only capture if selection is valid and reasonable size */
            if (selection_width > 5 && selection_height > 5 && 
                selection_width < 2000 && selection_height < 2000) {
                
                int buffer_size = selection_width * selection_height * 3;
                
                /* Free old buffer if exists */
                if (selection_buffer != NULL) {
                    free(selection_buffer);
                }
                
                selection_buffer = (unsigned char*)malloc(buffer_size);
                
                if (selection_buffer != NULL) {
                    int read_y = wh - max_y;
                    glReadPixels(min_x, read_y, selection_width, selection_height, 
                                GL_RGB, GL_UNSIGNED_BYTE, selection_buffer);
                    
                    select_has_content = 1;
                    selection_origin_x = min_x;
                    selection_origin_y = min_y;
                    move_offset_x = 0;
                    move_offset_y = 0;
                    
                    printf("Selection captured: %dx%d at (%d,%d)\n", 
                           selection_width, selection_height, min_x, min_y);
                } else {
                    printf("Failed to allocate selection buffer\n");
                    select_has_content = 0;
                }
            } else {
                printf("Invalid selection size: %dx%d\n", selection_width, selection_height);
                select_has_content = 0;
            }
            
            return;
        }
        
        /* Stop moving selection - REMOVED, now handled above */
        
        /* Stop brush drawing and save to history */
        if (brush_drawing && draw_mode == DRAW_POINTS && brush_start) {
            brush_drawing = 0;
            brush_start = 0;
            /* Complete brush stroke in history */
            if (history_count < MAX_HISTORY) {
                history_count++;
                redo_count = 0;
            }
        }
        
        /* Stop spray and save to history */
        if (spray_mode && spray_start) {
            spray_start = 0;
            /* Complete spray stroke in history */
            if (history_count < MAX_HISTORY && history[history_count].point_count > 0) {
                history_count++;
                redo_count = 0;
            }
        }
        
        /* Stop eraser and save to history */
        if (eraser_mode && eraser_start) {
            eraser_start = 0;
            /* Complete eraser stroke in history */
            if (history_count < MAX_HISTORY && history[history_count].point_count > 0) {
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
                    history[history_count].fill_data = NULL;
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
                    history[history_count].fill_data = NULL;
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

/* Improved scanline flood fill algorithm */
void scanlineFill(int x, int y, float old_r, float old_g, float old_b) {
    float tolerance = 0.01f;
    int maxY = wh - ww / 13;  /* Don't fill toolbar area */
    
    /* Check if starting point is valid */
    if (x < 0 || x >= ww || y < 0 || y >= maxY) return;
    
    /* Check if already the target color */
    if (fabs(old_r - r) < tolerance && fabs(old_g - g) < tolerance && fabs(old_b - b) < tolerance) {
        return;  /* Already the same color */
    }
    
    /* Create visited array */
    static unsigned char *visited = NULL;
    static int visited_size = 0;
    int required_size = ww * maxY;
    
    if (visited == NULL || visited_size < required_size) {
        if (visited != NULL) free(visited);
        visited = (unsigned char*)calloc(required_size, sizeof(unsigned char));
        visited_size = required_size;
        if (visited == NULL) {
            printf("Failed to allocate visited array\n");
            return;
        }
    } else {
        memset(visited, 0, required_size * sizeof(unsigned char));
    }
    
    /* Initialize stack */
    stackTop = -1;
    fillStack[++stackTop].x = x;
    fillStack[stackTop].y = y;
    
    glColor3f(r, g, b);
    
    int points_filled = 0;
    
    /* Process stack */
    while (stackTop >= 0) {
        /* Prevent stack overflow */
        if (stackTop >= STACK_SIZE - 100) {
            printf("Stack overflow in fill - stopping\n");
            break;
        }
        
        Point p = fillStack[stackTop--];
        int px = p.x;
        int py = p.y;
        
        /* Boundary check */
        if (px < 0 || px >= ww || py < 0 || py >= maxY) continue;
        
        /* Check if already visited */
        int idx = py * ww + px;
        if (idx < 0 || idx >= required_size) continue;
        if (visited[idx]) continue;
        
        /* Read current pixel color */
        GLubyte pixel[3];
        glReadPixels(px, py, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
        float curr_r = pixel[0] / 255.0f;
        float curr_g = pixel[1] / 255.0f;
        float curr_b = pixel[2] / 255.0f;
        
        /* Check if color matches the original color */
        if (fabs(curr_r - old_r) > tolerance || 
            fabs(curr_g - old_g) > tolerance || 
            fabs(curr_b - old_b) > tolerance) {
            continue;
        }
        
        /* Mark as visited and fill */
        visited[idx] = 1;
        
        /* Scanline fill - extend left and right */
        int left = px;
        int right = px;
        
        /* Extend to the left */
        while (left > 0) {
            int test_idx = py * ww + (left - 1);
            if (test_idx < 0 || test_idx >= required_size || visited[test_idx]) break;
            
            glReadPixels(left - 1, py, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
            curr_r = pixel[0] / 255.0f;
            curr_g = pixel[1] / 255.0f;
            curr_b = pixel[2] / 255.0f;
            
            if (fabs(curr_r - old_r) > tolerance || 
                fabs(curr_g - old_g) > tolerance || 
                fabs(curr_b - old_b) > tolerance) break;
            
            left--;
            visited[test_idx] = 1;
        }
        
        /* Extend to the right */
        while (right < ww - 1) {
            int test_idx = py * ww + (right + 1);
            if (test_idx < 0 || test_idx >= required_size || visited[test_idx]) break;
            
            glReadPixels(right + 1, py, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
            curr_r = pixel[0] / 255.0f;
            curr_g = pixel[1] / 255.0f;
            curr_b = pixel[2] / 255.0f;
            
            if (fabs(curr_r - old_r) > tolerance || 
                fabs(curr_g - old_g) > tolerance || 
                fabs(curr_b - old_b) > tolerance) break;
            
            right++;
            visited[test_idx] = 1;
        }
        
        /* Draw the horizontal line */
        glBegin(GL_POINTS);
        for (int i = left; i <= right; i++) {
            glVertex2i(i, py);
            points_filled++;
        }
        glEnd();
        
        /* Flush periodically for visual feedback */
        if (points_filled % 500 == 0) {
            glFlush();
        }
        
        /* Add adjacent rows to stack */
        for (int i = left; i <= right; i++) {
            /* Check row above */
            if (py + 1 < maxY && stackTop < STACK_SIZE - 2) {
                int above_idx = (py + 1) * ww + i;
                if (above_idx >= 0 && above_idx < required_size && !visited[above_idx]) {
                    fillStack[++stackTop].x = i;
                    fillStack[stackTop].y = py + 1;
                }
            }
            
            /* Check row below */
            if (py - 1 >= 0 && stackTop < STACK_SIZE - 2) {
                int below_idx = (py - 1) * ww + i;
                if (below_idx >= 0 && below_idx < required_size && !visited[below_idx]) {
                    fillStack[++stackTop].x = i;
                    fillStack[stackTop].y = py - 1;
                }
            }
        }
    }
    
    glFlush();
    printf("Filled %d points\n", points_filled);
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
                    /* Draw all points in the brush stroke */
                    for (int j = 0; j < history[i].point_count; j++) {
                        glBegin(GL_POLYGON);
                        glVertex2f(history[i].points[j][0] + s, history[i].points[j][1] + s);
                        glVertex2f(history[i].points[j][0] - s, history[i].points[j][1] + s);
                        glVertex2f(history[i].points[j][0] - s, history[i].points[j][1] - s);
                        glVertex2f(history[i].points[j][0] + s, history[i].points[j][1] - s);
                        glEnd();
                    }
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
                    /* Draw all points in the eraser stroke */
                    for (int j = 0; j < history[i].point_count; j++) {
                        glBegin(GL_POLYGON);
                        glVertex2f(history[i].points[j][0] + s, history[i].points[j][1] + s);
                        glVertex2f(history[i].points[j][0] - s, history[i].points[j][1] + s);
                        glVertex2f(history[i].points[j][0] - s, history[i].points[j][1] - s);
                        glVertex2f(history[i].points[j][0] + s, history[i].points[j][1] - s);
                        glEnd();
                    }
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
                
            case FILL_BUCKET:
                /* Restore the filled area from saved data */
                if (history[i].fill_data != NULL) {
                    /* First restore the before-fill state, then apply fill */
                    /* Since we're redrawing everything, we need to apply the fill again */
                    /* This is a simplified version - just skip for now during redraw */
                }
                break;
                
            case SELECT:
                /* Restore selected area with safety checks */
                if (history[i].fill_data != NULL && 
                    history[i].fill_width > 0 && history[i].fill_height > 0 &&
                    history[i].x1 >= 0 && history[i].x1 < ww &&
                    history[i].y1 >= 0 && history[i].y1 < wh) {
                    
                    int draw_y = wh - history[i].y1 - history[i].fill_height;
                    if (draw_y >= 0 && draw_y < wh && 
                        history[i].x1 + history[i].fill_width <= ww) {
                        glRasterPos2i(history[i].x1, draw_y);
                        glDrawPixels(history[i].fill_width, history[i].fill_height, 
                                    GL_RGB, GL_UNSIGNED_BYTE, history[i].fill_data);
                    }
                }
                break;
        }
    }
    glFlush();
}

/* Undo */
void undo(void) {
    if (history_count > 0) {
        /* Move current item to redo stack */
        if (redo_count < MAX_HISTORY) {
            redo_stack[redo_count] = history[history_count - 1];
            redo_count++;
        }
        history_count--;
        redrawHistory();
    }
}

/* Redo */
void redo(void) {
    if (redo_count > 0) {
        /* Move item from redo stack back to history */
        if (history_count < MAX_HISTORY) {
            history[history_count] = redo_stack[redo_count - 1];
            history_count++;
            redo_count--;
            redrawHistory();
        }
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
        
        /* Free selection buffer and reset selection state */
        if (selection_buffer != NULL) {
            free(selection_buffer);
            selection_buffer = NULL;
        }
        selection_width = 0;
        selection_height = 0;
        select_has_content = 0;
        selecting = 0;
        moving_selection = 0;
        move_offset_x = 0;
        move_offset_y = 0;
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
        
        /* Free selection buffer and reset selection state */
        if (selection_buffer != NULL) {
            free(selection_buffer);
            selection_buffer = NULL;
        }
        selection_width = 0;
        selection_height = 0;
        select_has_content = 0;
        selecting = 0;
        moving_selection = 0;
        move_offset_x = 0;
        move_offset_y = 0;
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
    
    /* Bezier Surface button - in top right corner */
    int bezier_btn_size = 80;
    int bezier_x = ww - bezier_btn_size - 10;
    int bezier_y = 10;
    
    if (show_bezier) {
        glColor3f(0.2, 0.8, 0.2); /* Green when active */
    } else {
        glColor3f(0.6, 0.6, 1.0); /* Blue when inactive */
    }
    glBegin(GL_QUADS);
    glVertex2i(bezier_x, wh - bezier_y);
    glVertex2i(bezier_x + bezier_btn_size, wh - bezier_y);
    glVertex2i(bezier_x + bezier_btn_size, wh - bezier_y - bezier_btn_size);
    glVertex2i(bezier_x, wh - bezier_y - bezier_btn_size);
    glEnd();
    
    /* Draw Bezier curve icon */
    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float t = (float)i / 20.0;
        float x = bezier_x + bezier_btn_size * 0.2 + 
                  bezier_btn_size * 0.6 * (3 * (1-t) * (1-t) * t * 0.3 + 
                                           3 * (1-t) * t * t * 0.7 + 
                                           t * t * t);
        float y = wh - bezier_y - bezier_btn_size * 0.8 + 
                  bezier_btn_size * 0.6 * (3 * (1-t) * (1-t) * t * 0.7 + 
                                           3 * (1-t) * t * t * 0.3);
        glVertex2f(x, y);
    }
    glEnd();
    
    /* Draw text "Bezier" */
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(bezier_x + 10, wh - bezier_y - 20);
    const char* bezier_text = "Bezier";
    for (int i = 0; bezier_text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, bezier_text[i]);
    }
    glRasterPos2i(bezier_x + 8, wh - bezier_y - 35);
    const char* surface_text = "Surface";
    for (int i = 0; surface_text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, surface_text[i]);
    }
    glLineWidth(1.0);
    
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

/* Bernstein polynomial */
float bernstein(int i, int n, float u) {
    float binomial = 1.0;
    for (int k = 0; k < i; k++) {
        binomial *= (float)(n - k) / (float)(k + 1);
    }
    return binomial * pow(u, i) * pow(1.0 - u, n - i);
}

/* Compute point on Bezier surface */
void computeBezierSurface(float u, float v, float* point) {
    point[0] = point[1] = point[2] = 0.0;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float bu = bernstein(i, 3, u);
            float bv = bernstein(j, 3, v);
            point[0] += controlPoints[i][j][0] * bu * bv;
            point[1] += controlPoints[i][j][1] * bu * bv;
            point[2] += controlPoints[i][j][2] * bu * bv;
        }
    }
}

/* Initialize Bezier surface control points */
void initBezierSurface(void) {
    /* Initialize control points in a grid */
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            controlPoints[i][j][0] = (i - 1.5) * 2.0;
            controlPoints[i][j][1] = (j - 1.5) * 2.0;
            controlPoints[i][j][2] = 0.0;
        }
    }
    
    /* Add some initial curvature */
    controlPoints[1][1][2] = 1.5;
    controlPoints[1][2][2] = 1.5;
    controlPoints[2][1][2] = 1.5;
    controlPoints[2][2][2] = 1.5;
}

/* Display Bezier surface */
void displayBezierSurface(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    
    glRotatef(rotateX, 1.0, 0.0, 0.0);
    glRotatef(rotateY, 0.0, 1.0, 0.0);
    
    /* Draw Bezier surface */
    glColor3f(0.3, 0.6, 0.9);
    int divisions = 20;
    for (int i = 0; i < divisions; i++) {
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= divisions; j++) {
            float u1 = (float)i / divisions;
            float u2 = (float)(i + 1) / divisions;
            float v = (float)j / divisions;
            
            float point1[3], point2[3];
            computeBezierSurface(u1, v, point1);
            computeBezierSurface(u2, v, point2);
            
            glVertex3fv(point1);
            glVertex3fv(point2);
        }
        glEnd();
    }
    
    /* Draw control points */
    glPointSize(8.0);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            glVertex3fv(controlPoints[i][j]);
        }
    }
    glEnd();
    
    /* Draw control mesh */
    glColor3f(0.5, 0.5, 0.5);
    glLineWidth(1.0);
    for (int i = 0; i < 4; i++) {
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < 4; j++) {
            glVertex3fv(controlPoints[i][j]);
        }
        glEnd();
    }
    for (int j = 0; j < 4; j++) {
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < 4; i++) {
            glVertex3fv(controlPoints[i][j]);
        }
        glEnd();
    }
    
    glutSwapBuffers();
}

/* Display control window */
void displayControlWindow(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* Draw control point grid (top view) */
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_QUADS);
    glVertex2f(-8.0, -8.0);
    glVertex2f(8.0, -8.0);
    glVertex2f(8.0, 8.0);
    glVertex2f(-8.0, 8.0);
    glEnd();
    
    /* Draw grid lines */
    glColor3f(0.6, 0.6, 0.6);
    glLineWidth(1.0);
    for (int i = -8; i <= 8; i += 2) {
        glBegin(GL_LINES);
        glVertex2f(i, -8.0);
        glVertex2f(i, 8.0);
        glVertex2f(-8.0, i);
        glVertex2f(8.0, i);
        glEnd();
    }
    
    /* Draw control mesh (top view projection) */
    glColor3f(0.3, 0.3, 0.3);
    glLineWidth(2.0);
    for (int i = 0; i < 4; i++) {
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < 4; j++) {
            glVertex2f(controlPoints[i][j][0], controlPoints[i][j][1]);
        }
        glEnd();
    }
    for (int j = 0; j < 4; j++) {
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < 4; i++) {
            glVertex2f(controlPoints[i][j][0], controlPoints[i][j][1]);
        }
        glEnd();
    }
    
    /* Draw control points */
    glPointSize(10.0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (selectedPoint >= 0 && selectedI == i && selectedJ == j) {
                glColor3f(1.0, 1.0, 0.0); /* Selected point in yellow */
            } else {
                /* Color based on Z height */
                float z = controlPoints[i][j][2];
                glColor3f(0.0, 0.5 + z * 0.2, 1.0 - z * 0.2);
            }
            glBegin(GL_POINTS);
            glVertex2f(controlPoints[i][j][0], controlPoints[i][j][1]);
            glEnd();
        }
    }
    
    /* Draw instructions */
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2f(-7.5, 7.0);
    const char* text1 = "Drag points to adjust surface";
    for (int i = 0; text1[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text1[i]);
    }
    
    glRasterPos2f(-7.5, 6.3);
    const char* text2 = "Use +/- keys to adjust Z height";
    for (int i = 0; text2[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text2[i]);
    }
    
    glutSwapBuffers();
}

/* Reshape Bezier surface window */
void reshapeBezierSurface(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

/* Reshape control window */
void reshapeControlWindow(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-8.0, 8.0, -8.0, 8.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

/* Mouse handler for Bezier surface window */
void mouseBezierSurface(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            lastMouseX = x;
            lastMouseY = y;
        }
    }
}

/* Motion handler for Bezier surface window */
void motionBezierSurface(int x, int y) {
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;
    
    rotateY += dx * 0.5;
    rotateX += dy * 0.5;
    
    lastMouseX = x;
    lastMouseY = y;
    
    glutPostRedisplay();
}

/* Mouse handler for control window */
void mouseControlWindow(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            /* Convert screen coordinates to world coordinates */
            int width = glutGet(GLUT_WINDOW_WIDTH);
            int height = glutGet(GLUT_WINDOW_HEIGHT);
            float wx = ((float)x / width) * 16.0 - 8.0;
            float wy = 8.0 - ((float)y / height) * 16.0;
            
            /* Find nearest control point */
            float minDist = 0.5;
            selectedPoint = -1;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    float dx = wx - controlPoints[i][j][0];
                    float dy = wy - controlPoints[i][j][1];
                    float dist = sqrt(dx * dx + dy * dy);
                    if (dist < minDist) {
                        minDist = dist;
                        selectedPoint = i * 4 + j;
                        selectedI = i;
                        selectedJ = j;
                    }
                }
            }
            
            glutSetWindow(control_window);
            glutPostRedisplay();
        } else if (state == GLUT_UP) {
            selectedPoint = -1;
            glutSetWindow(control_window);
            glutPostRedisplay();
        }
    }
}

/* Motion handler for control window */
void motionControlWindow(int x, int y) {
    if (selectedPoint >= 0) {
        /* Convert screen coordinates to world coordinates */
        int width = glutGet(GLUT_WINDOW_WIDTH);
        int height = glutGet(GLUT_WINDOW_HEIGHT);
        float wx = ((float)x / width) * 16.0 - 8.0;
        float wy = 8.0 - ((float)y / height) * 16.0;
        
        /* Clamp to bounds */
        if (wx < -8.0) wx = -8.0;
        if (wx > 8.0) wx = 8.0;
        if (wy < -8.0) wy = -8.0;
        if (wy > 8.0) wy = 8.0;
        
        /* Update control point position */
        controlPoints[selectedI][selectedJ][0] = wx;
        controlPoints[selectedI][selectedJ][1] = wy;
        
        /* Update both windows */
        glutSetWindow(control_window);
        glutPostRedisplay();
        glutSetWindow(bezier_surface_window);
        glutPostRedisplay();
    }
}

/* Keyboard handler for Bezier surface window */
void keyBezierSurface(unsigned char key, int x, int y) {
    if (key == '+' || key == '=') {
        if (selectedPoint >= 0) {
            controlPoints[selectedI][selectedJ][2] += 0.2;
            glutSetWindow(control_window);
            glutPostRedisplay();
            glutSetWindow(bezier_surface_window);
            glutPostRedisplay();
        }
    } else if (key == '-' || key == '_') {
        if (selectedPoint >= 0) {
            controlPoints[selectedI][selectedJ][2] -= 0.2;
            glutSetWindow(control_window);
            glutPostRedisplay();
            glutSetWindow(bezier_surface_window);
            glutPostRedisplay();
        }
    } else if (key == 27) { /* ESC key */
        if (bezier_surface_window) {
            glutDestroyWindow(bezier_surface_window);
            bezier_surface_window = 0;
        }
        if (control_window) {
            glutDestroyWindow(control_window);
            control_window = 0;
        }
        show_bezier = 0;
    }
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