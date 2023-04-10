#include <GL/gl.h>
#include <cstdio>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cmath>

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 512;
constexpr float PI2 = 2*M_PI;

struct Player {
    float x;
    float y;
    float angle;
    float dx;
    float dy;

    void draw() {
        glColor3f(1,1,0);
        glPointSize(8);
        glBegin(GL_POINTS);
        glVertex2i(x, y);
        glEnd();

        glLineWidth(3);
        glBegin(GL_LINES);
        glVertex2i(x,y);
        glVertex2i(x+(dx*5),y+(dy*5));
        glEnd();
    }
    void turnLeft() {
        angle -= 0.1;
        if(angle < 0) {
            angle += PI2;
        }
        updateDirection();
    }
    void turnRight() {
        angle += 0.1;
        if(angle > PI2) {
            angle -= PI2;
        }
        updateDirection();
    }
    void moveForward() {
        x += dx;
        y += dy;
    }
    void moveBackward() {
        x -= dx;
        y -= dy;
    }
    private:
    inline void updateDirection() {
        dx = cosf(angle) * 5;
        dy = sinf(angle) * 5;
    }
};
Player player{300,300,0,5,0};

constexpr int MAP_WIDTH = 8;
constexpr int MAP_HEIGHT = 8;
constexpr int CELL_SIZE = 64;
constexpr unsigned char map[MAP_WIDTH][MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,
};

static void drawMap2D() {
    for(int y = 0; y < MAP_HEIGHT; ++y) {
        for(int x = 0; x < MAP_WIDTH; ++x) {
            if (map[x][y]) {
                glColor3f(1,1,1);
            } else {
                glColor3f(0,0,0);
            }
            glBegin(GL_QUADS);
            glVertex2i(x*CELL_SIZE+1, y*CELL_SIZE+1);
            glVertex2i(x*CELL_SIZE+1, (y+1)*CELL_SIZE-1);
            glVertex2i((x+1)*CELL_SIZE-1, (y+1)*CELL_SIZE-1);
            glVertex2i((x+1)*CELL_SIZE-1, y*CELL_SIZE+1);
            glEnd();
        }
    }

}

struct Point {
    float x;
    float y;
};

constexpr float distance(Point p1, Point p2) {
    return hypotf(p1.x - p2.x,p1.y - p2.y);

}

constexpr float EPSILON = 0.0001;
constexpr int DEPTH_OF_FIELD = 8;

static bool cast_line_horizontal(float ray_angle, Point& point_hit) {
        float a_tan = -1/tan(ray_angle);
        point_hit.y = player.y;
        point_hit.x = player.x;
        float y_offset=0.0;
        float x_offset=0.0;
        if (ray_angle == 0 || ray_angle == float(M_PI)) {
            return false;
        }
        if (ray_angle > M_PI) {
            point_hit.y = std::floor(player.y/CELL_SIZE) * CELL_SIZE - EPSILON;
            point_hit.x = (player.y - point_hit.y) * a_tan + player.x;
            y_offset=-64;
            x_offset=-y_offset * a_tan;
        } else if (ray_angle < M_PI) {
            point_hit.y = (std::floor(player.y/CELL_SIZE) + 1) * CELL_SIZE;
            point_hit.x = (player.y - point_hit.y) * a_tan + player.x;
            y_offset=64;
            x_offset=-y_offset * a_tan;
        }
        for(int iter_depth = 0;iter_depth < DEPTH_OF_FIELD;iter_depth++){
            int ray_hit_cell_x = int(point_hit.x / 64);
            int ray_hit_cell_y = int(point_hit.y / 64);
            if (ray_hit_cell_x >= 0 && ray_hit_cell_y >= 0 &&
                ray_hit_cell_x < MAP_WIDTH && ray_hit_cell_y < MAP_HEIGHT &&
                map[ray_hit_cell_x][ray_hit_cell_y]) {
                iter_depth = DEPTH_OF_FIELD;
                return true;
            } else {
                point_hit.x += x_offset;
                point_hit.y += y_offset;
            }
        }
        return false;
}

static bool cast_line_vertical(float ray_angle, Point& point_hit) {
        float n_tan = -tan(ray_angle);
        point_hit.y = player.y;
        point_hit.x = player.x;
        float y_offset=0.0;
        float x_offset=0.0;
        int iter_depth = 0;
        if(ray_angle == M_PI / 2 || ray_angle == 3 * M_PI / 2) {
            return false;
        }
        if (ray_angle > M_PI / 2 && ray_angle < 3 * M_PI / 2) {
            point_hit.x = std::floor(player.x/CELL_SIZE) * CELL_SIZE - EPSILON;
            x_offset=-64;
        } else if (ray_angle < M_PI / 2 || ray_angle > 3 * M_PI / 2) {
            point_hit.x = (std::floor(player.x/CELL_SIZE) + 1) * CELL_SIZE;
            x_offset=64;
        }
        point_hit.y = (player.x - point_hit.x) * n_tan + player.y;
        y_offset=-x_offset * n_tan;
        for(int iter_depth = 0;iter_depth < DEPTH_OF_FIELD;iter_depth++){
            int ray_hit_cell_x = int(point_hit.x / 64);
            int ray_hit_cell_y = int(point_hit.y / 64);
            if (ray_hit_cell_x >= 0 && ray_hit_cell_y >= 0 &&
                ray_hit_cell_x < MAP_WIDTH && ray_hit_cell_y < MAP_HEIGHT &&
                map[ray_hit_cell_x][ray_hit_cell_y]) {
                iter_depth = DEPTH_OF_FIELD;
                return true;
            } else {
                point_hit.x += x_offset;
                point_hit.y += y_offset;
            }
        }
        return false;
}

static float cast_ray(float angle, Point& hit) {
    bool did_hit = false;
    Point p{player.x, player.y};
    Point hitv{0, 0};
    Point hith{0, 0};
    float dist = 0.0;
    if(cast_line_horizontal(angle, hith)){
        hit = hith;
        did_hit = true;
        dist = distance(hith, p);
    }
    if(cast_line_vertical(angle, hitv)){
        float v_dist = distance(hitv, p);
        if(did_hit) {
            if(v_dist < dist) {
                hit = hitv;
                dist = v_dist;
                return dist;
            }
        } else {
            hit = hitv;
            dist = v_dist;
            return dist;
        }
    }
    return dist;
}

static inline constexpr float limit_angle(float angle) {
        if(angle < 0){
            angle += PI2;
        }
        if(angle > PI2){
            angle -= PI2;
        }
        return angle;
}

constexpr int NUM_RAYS = 60;
constexpr float FIELD_OF_VIEW = 0.3; // Radians
constexpr float DELTA_ANGLE = FIELD_OF_VIEW * 2 / NUM_RAYS;
static void drawWalls() {
    Point hit{0,0};
    float ray_angle = player.angle - FIELD_OF_VIEW;
    for (int r = 0; r < NUM_RAYS; ++r) {
        ray_angle = limit_angle(ray_angle + DELTA_ANGLE);
        Point hit{0, 0};
        float dist = cast_ray(ray_angle, hit);

        // Fix fisheye
        float diff_angle = limit_angle(ray_angle - player.angle);
        dist *= cosf(diff_angle);


        float line_height = (MAP_WIDTH * MAP_HEIGHT * HEIGHT) / dist;
        if(line_height > HEIGHT) {
            line_height = HEIGHT;
        }
        float line_offset = (HEIGHT - line_height) / 2;
        glColor3f(1,0,0);
        glLineWidth(8);
        glBegin(GL_LINES);
        glVertex2i(r*8+530,line_offset);
        glVertex2i(r*8+530,line_height + line_offset);
        glEnd();
    }
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawMap2D();
    player.draw();
    drawWalls();
    glutSwapBuffers();
}

static void init() {
    glClearColor(0.3,0.3,0.3,0);
    gluOrtho2D(0,WIDTH,HEIGHT,0);
}

static void handleButtonPress(unsigned char key, int x, int y) {
    if(key == 'd') { player.turnRight(); }
    if(key == 'a') { player.turnLeft(); }
    if(key == 'w') { player.moveForward(); }
    if(key == 's') { player.moveBackward(); }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(handleButtonPress);
    glutMainLoop();
}
