#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

int window_width = 1024;
int window_height = 512;
constexpr float PI2 = 2 * M_PI;

constexpr int MAP_WIDTH = 8;
constexpr int MAP_HEIGHT = 8;
constexpr int CELL_SIZE = 64;
constexpr unsigned char map[MAP_WIDTH][MAP_HEIGHT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0,
    0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

struct Player {
    float x;
    float y;
    float angle;
    float dx;
    float dy;

    bool turning_left;
    bool turning_right;
    bool moving_forward;
    bool moving_backward;

    void draw() {
        glColor3f(1, 1, 0);
        glPointSize(8);
        glBegin(GL_POINTS);
        glVertex2i(x, y);
        glEnd();

        if (moving_forward)
            moveForward();
        if (moving_backward)
            moveBackward();
        if (turning_left)
            turnLeft();
        if (turning_right)
            turnRight();
        if (moving_forward || moving_backward || turning_left || turning_right)
            glutPostRedisplay();

        glLineWidth(3);
        glBegin(GL_LINES);
        glVertex2i(x, y);
        glVertex2i(x + (dx * 5), y + (dy * 5));
        glEnd();
    }
    void turnLeft() {
        angle -= 0.1;
        if (angle < 0) {
            angle += PI2;
        }
        updateDirection();
    }
    void turnRight() {
        angle += 0.1;
        if (angle > PI2) {
            angle -= PI2;
        }
        updateDirection();
    }
    void moveForward() {
        int map_x = int(x / CELL_SIZE);
        int map_y = int(y / CELL_SIZE);
        if(map[int((x+dx)/CELL_SIZE)][map_y] == 0) {
            x += dx;
        }
        if(map[map_x][int((y + dy) / CELL_SIZE)] == 0) {
            y += dy;
        }
    }
    void moveBackward() {
        int map_x = int(x / CELL_SIZE);
        int map_y = int(y / CELL_SIZE);
        if(map[int((x-dx)/CELL_SIZE)][map_y] == 0) {
            x -= dx;
        }
        if(map[map_x][int((y - dy) / CELL_SIZE)] == 0) {
            y -= dy;
        }
    }

private:
    inline void updateDirection() {
        dx = cosf(angle) * 5;
        dy = sinf(angle) * 5;
    }
};
Player player{300, 300, 0, 5, 0, false, false, false, false};

static void drawMap2D() {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (map[x][y]) {
                glColor3f(1, 1, 1);
            } else {
                glColor3f(0, 0, 0);
            }
            glBegin(GL_QUADS);
            glVertex2i(x * CELL_SIZE + 1, y * CELL_SIZE + 1);
            glVertex2i(x * CELL_SIZE + 1, (y + 1) * CELL_SIZE - 1);
            glVertex2i((x + 1) * CELL_SIZE - 1, (y + 1) * CELL_SIZE - 1);
            glVertex2i((x + 1) * CELL_SIZE - 1, y * CELL_SIZE + 1);
            glEnd();
        }
    }
}

struct Point {
    float x;
    float y;
};

constexpr float distance(Point p1, Point p2) {
    return hypotf(p1.x - p2.x, p1.y - p2.y);
}

constexpr float EPSILON = 0.0001;
constexpr int DEPTH_OF_FIELD = 8;

static bool cast_line_horizontal(float ray_angle, Point &point_hit) {
    float a_tan = -1 / tan(ray_angle);
    point_hit.y = player.y;
    point_hit.x = player.x;
    float y_offset = 0.0;
    float x_offset = 0.0;
    if (ray_angle == 0 || ray_angle == float(M_PI)) {
        return false;
    }
    if (ray_angle > M_PI) {
        point_hit.y = std::floor(player.y / CELL_SIZE) * CELL_SIZE - EPSILON;
        point_hit.x = (player.y - point_hit.y) * a_tan + player.x;
        y_offset = -64;
        x_offset = -y_offset * a_tan;
    } else if (ray_angle < M_PI) {
        point_hit.y = (std::floor(player.y / CELL_SIZE) + 1) * CELL_SIZE;
        point_hit.x = (player.y - point_hit.y) * a_tan + player.x;
        y_offset = 64;
        x_offset = -y_offset * a_tan;
    }
    for (int iter_depth = 0; iter_depth < DEPTH_OF_FIELD; iter_depth++) {
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

static bool cast_line_vertical(float ray_angle, Point &point_hit) {
    float n_tan = -tan(ray_angle);
    point_hit.y = player.y;
    point_hit.x = player.x;
    float y_offset = 0.0;
    float x_offset = 0.0;
    int iter_depth = 0;
    if (ray_angle == M_PI / 2 || ray_angle == 3 * M_PI / 2) {
        return false;
    }
    if (ray_angle > M_PI / 2 && ray_angle < 3 * M_PI / 2) {
        point_hit.x = std::floor(player.x / CELL_SIZE) * CELL_SIZE - EPSILON;
        x_offset = -64;
    } else if (ray_angle < M_PI / 2 || ray_angle > 3 * M_PI / 2) {
        point_hit.x = (std::floor(player.x / CELL_SIZE) + 1) * CELL_SIZE;
        x_offset = 64;
    }
    point_hit.y = (player.x - point_hit.x) * n_tan + player.y;
    y_offset = -x_offset * n_tan;
    for (int iter_depth = 0; iter_depth < DEPTH_OF_FIELD; iter_depth++) {
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

static float cast_ray(float angle, Point &hit) {
    bool did_hit = false;
    Point p{player.x, player.y};
    Point hitv{0, 0};
    Point hith{0, 0};
    float dist = 0.0;
    if (cast_line_horizontal(angle, hith)) {
        hit = hith;
        did_hit = true;
        dist = distance(hith, p);
        glColor3f(0.9, 0, 0);
    }
    if (cast_line_vertical(angle, hitv)) {
        float v_dist = distance(hitv, p);
        if (did_hit) {
            if (v_dist < dist) {
                glColor3f(0.7, 0, 0);
                hit = hitv;
                dist = v_dist;
                return dist;
            }
        } else {
            glColor3f(0.7, 0, 0);
            hit = hitv;
            dist = v_dist;
            return dist;
        }
    }
    return dist;
}

static inline constexpr float limitAngle(float angle) {
    return std::fmod(angle, PI2);
}

constexpr int NUM_RAYS = 60;
constexpr float FIELD_OF_VIEW = 0.3; // Radians
constexpr float DELTA_ANGLE = FIELD_OF_VIEW * 2 / NUM_RAYS;
constexpr int LINE_WIDTH = 8;
constexpr int PADDING = 10;
static void drawWalls() {
    float ray_angle = player.angle - FIELD_OF_VIEW;
    for (int r = 0; r < NUM_RAYS; ++r) {
        ray_angle = limitAngle(ray_angle + DELTA_ANGLE);
        Point hit{0, 0};
        float dist = cast_ray(ray_angle, hit);

        // Fix fisheye
        float diff_angle = limitAngle(ray_angle - player.angle);
        dist *= cosf(diff_angle);

        float line_height =
            std::clamp((MAP_WIDTH * MAP_HEIGHT * MAP_HEIGHT * CELL_SIZE) / dist,
                       0.0f, static_cast<float>(window_height));
        float line_offset = (MAP_HEIGHT * CELL_SIZE) / 2.0;
        glLineWidth(LINE_WIDTH);
        glBegin(GL_LINES);
        glVertex2i(r * LINE_WIDTH + MAP_WIDTH * CELL_SIZE + PADDING,
                   line_offset - line_height);
        glVertex2i(r * LINE_WIDTH + MAP_WIDTH * CELL_SIZE + PADDING,
                   line_offset + line_height);
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
    glClearColor(0.3, 0.3, 0.3, 0);
    gluOrtho2D(0, window_width, window_height, 0);
}

static void handleButtonPress(unsigned char key, int x, int y) {
    player.turning_right |= (key == 'd');
    player.turning_left |= (key == 'a');
    player.moving_forward |= (key == 'w');
    player.moving_backward |= (key == 's');
    glutPostRedisplay();
}

static void handleButtonUp(unsigned char key, int x, int y) {
    player.turning_right &= (key != 'd');
    player.turning_left &= (key != 'a');
    player.moving_forward &= (key != 'w');
    player.moving_backward &= (key != 's');
    glutPostRedisplay();
}

static void resize(int w, int h) {
    window_width = w;
    window_height = h;
    glutPostRedisplay();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(handleButtonPress);
    glutKeyboardUpFunc(handleButtonUp);
    glutMainLoop();
}
