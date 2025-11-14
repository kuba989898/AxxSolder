// Display of temperature and power graph

#define GRAPH_X0  34
#define GRAPH_Y0  217
#define GRAPH_WIDTH  250
#define GRAPH_HEIGHT 130

#define TEMP_MIN 0
#define TEMP_MAX 450
#define POWER_MIN 0
#define POWER_MAX 100

#define GRAPH_POINTS 100

#define GRAPH_Color_TEMP    B_ZARQA        // Temperature graph color
#define GRAPH_Color_POWER   B_LAWN_GREEN   // Power graph color

#define DASH_LEN_COLOR   B_DARK_GRAY   // Coordinate grid dashed line color
#define DASH_LEN 5                     // Dash length in pixels
#define GAP_LEN  5                     // Gap length in pixels
#define THICKNESS_LEN  1               // Line thickness in pixels

uint16_t temp_array[GRAPH_POINTS];
uint16_t power_array[GRAPH_POINTS];
uint16_t prev_temp_array[GRAPH_POINTS];
uint16_t prev_power_array[GRAPH_POINTS];
uint8_t index_graph = 0;

static uint8_t initialized = 0;

// Structure for storing a line with its color
typedef struct {
    int x1, y1, x2, y2;
    uint16_t color;  // B_RED or B_BLUE or others
} Line;


void add_data_point(uint16_t temp, uint16_t power) {
    temp_array[index_graph] = temp;
    power_array[index_graph] = power;
    index_graph++;
    if (index_graph >= GRAPH_POINTS) index_graph = 0; // Circular buffer
}

static Line temp_lines_prev[GRAPH_POINTS - 1] = {0};
static Line power_lines_prev[GRAPH_POINTS - 1] = {0};

// Universal dashed line using UG_FillFrame()
// dash_len  – dash length in pixels
// gap_len   – gap length in pixels
// thickness – line thickness in pixels
void draw_dashed_line_fillframe(int x1, int y1, int x2, int y2, uint16_t color,
                                uint8_t dash_len, uint8_t gap_len, uint8_t thickness) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    uint8_t phase = 0;          // Position inside dash or gap
    uint8_t draw = 1;           // Draw or skip
    uint8_t segment_len = dash_len;

    while (1) {
        if (draw) {
            UG_FillFrame(x1, y1, x1 + (thickness - 1), y1 + (thickness - 1), color);
        }
        phase++;
        if (phase >= segment_len) {
            phase = 0;
            draw = !draw; // Switch drawing mode
            segment_len = draw ? dash_len : gap_len;
        }

        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void draw_graph_init(void) {
    // Background, grid, and labels are drawn once
    UG_FillFrame(GRAPH_X0, GRAPH_Y0 - GRAPH_HEIGHT , GRAPH_X0 + GRAPH_WIDTH, GRAPH_Y0, background);

    // Draw horizontal dashed lines
    for (int y = 0; y <= GRAPH_HEIGHT; y += GRAPH_HEIGHT / 5) {
        draw_dashed_line_fillframe(GRAPH_X0, GRAPH_Y0 - y, GRAPH_X0 + GRAPH_WIDTH, GRAPH_Y0 - y,
        		DASH_LEN_COLOR, DASH_LEN, GAP_LEN, THICKNESS_LEN); // dash=1, gap=5, thickness=1
    }

    // Draw vertical dashed lines
    for (int x = 0; x <= GRAPH_WIDTH; x += GRAPH_WIDTH / 5) {
        draw_dashed_line_fillframe(GRAPH_X0 + x, GRAPH_Y0 - GRAPH_HEIGHT, GRAPH_X0 + x, GRAPH_Y0,
        		DASH_LEN_COLOR, DASH_LEN, GAP_LEN, THICKNESS_LEN);
    }

    // Labels along the time axis (X, in seconds)
    float step_time_sec = ((float)GRAPH_WIDTH / 5.0f) * ((float)interval_display / 1000.0f);

    for (int i = 0; i <= 5; i++) {
        int x_pos = GRAPH_X0 + i * (GRAPH_WIDTH / 5);
        char buf[12];

        if (i == 0) {
            snprintf(buf, sizeof(buf), "0 s");
        } else {
            // Format time without decimals
            snprintf(buf, sizeof(buf), "%.0f", i * step_time_sec);
        }

        int y_pos = GRAPH_Y0 + 2;

        // For the last label, shift 10 pixels to the left
        int x_draw = (i == 5) ? (x_pos - 15) : x_pos;

        LCD_PutStr(x_draw, y_pos, buf, FONT_tahoma_18X22, B_PALE_GREEN, background);
    }

    initialized = 1;
}

// Universal function for a line with a given thickness
void draw_line_with_thickness(int x1, int y1, int x2, int y2, uint16_t color, uint8_t thickness) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        // Thickness in pixels (square brush)
        UG_FillFrame(x1, y1, x1 + (thickness - 1), y1 + (thickness - 1), color);

        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// Restore grid dash
void restore_grid_point(int x, int y) {
    // Horizontal grid lines
    for (int gy = 0; gy <= GRAPH_HEIGHT; gy += GRAPH_HEIGHT / 5) {
        int grid_y = GRAPH_Y0 - gy;
        if (abs(y - grid_y) <= THICKNESS_LEN) {
            int cycle_len = DASH_LEN + GAP_LEN;
            int dash_index = (x - GRAPH_X0) / cycle_len;
            int dash_start = GRAPH_X0 + dash_index * cycle_len;
            int dash_end = dash_start + DASH_LEN - 1;

            if (dash_end > GRAPH_X0 + GRAPH_WIDTH) dash_end = GRAPH_X0 + GRAPH_WIDTH;

            // Draw the whole dash
            UG_FillFrame(dash_start, grid_y,
                         dash_end,   grid_y,
						 DASH_LEN_COLOR);
            return;
        }
    }

    // Vertical grid lines
    for (int gx = 0; gx <= GRAPH_WIDTH; gx += GRAPH_WIDTH / 5) {
        int grid_x = GRAPH_X0 + gx;
        if (abs(x - grid_x) <= THICKNESS_LEN) {
            int cycle_len = DASH_LEN + GAP_LEN;
            int dash_index = (GRAPH_Y0 - y) / cycle_len;
            int dash_start = GRAPH_Y0 - dash_index * cycle_len;
            int dash_end = dash_start - (DASH_LEN - 1);

            if (dash_end < GRAPH_Y0 - GRAPH_HEIGHT) dash_end = GRAPH_Y0 - GRAPH_HEIGHT;

            UG_FillFrame(grid_x,
                         dash_end, grid_x,
                         dash_start,
						 DASH_LEN_COLOR);
            return;
        }
    }
}

// Erases a line 2 pixels thick and restores the grid
void erase_line_and_restore_grid(int x1, int y1, int x2, int y2) {
    // Erase with thickness of 2 pixels
    draw_line_with_thickness(x1, y1, x2, y2, background, 2);

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    while (1) {
        restore_grid_point(x1, y1);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}


// Check if two line segments overlap within rectangles, with margin (thickness)
bool lines_overlap(Line *l1, Line *l2, int thickness) {
    int l1_xmin = l1->x1 < l1->x2 ? l1->x1 : l1->x2;
    int l1_xmax = l1->x1 > l1->x2 ? l1->x1 : l1->x2;
    int l1_ymin = l1->y1 < l1->y2 ? l1->y1 : l1->y2;
    int l1_ymax = l1->y1 > l1->y2 ? l1->y1 : l1->y2;

    int l2_xmin = l2->x1 < l2->x2 ? l2->x1 : l2->x2;
    int l2_xmax = l2->x1 > l2->x2 ? l2->x1 : l2->x2;
    int l2_ymin = l2->y1 < l2->y2 ? l2->y1 : l2->y2;
    int l2_ymax = l2->y1 > l2->y2 ? l2->y1 : l2->y2;

    // Add thickness margin for "overlap"
    l1_xmin -= thickness;
    l1_xmax += thickness;
    l1_ymin -= thickness;
    l1_ymax += thickness;

    l2_xmin -= thickness;
    l2_xmax += thickness;
    l2_ymin -= thickness;
    l2_ymax += thickness;

    bool x_overlap = (l1_xmin <= l2_xmax) && (l1_xmax >= l2_xmin);
    bool y_overlap = (l1_ymin <= l2_ymax) && (l1_ymax >= l2_ymin);

    return x_overlap && y_overlap;
}


// Helper function for drawing an axis with labels
// values      - array of values for labels
// count       - number of labels in the array
// x           - horizontal position for label output
// is_left_axis - true if the axis is on the left (labels on the left), false if on the right
// unit_start  - suffix for the first label (e.g., "°C" or "%"), can be NULL
// unit_end    - suffix for the last label, can be NULL
void draw_axis(const int *values, int count, int x, bool is_left_axis,
               uint16_t color, const char *unit_start, const char *unit_end) {
    // Upper and lower Y boundaries so labels don't go outside the grid
    int y_min = GRAPH_Y0 - GRAPH_HEIGHT + 10; // slightly below the top grid line
    int y_max = GRAPH_Y0 - 5;                 // slightly above the bottom grid line

    for (int i = 0; i < count; i++) {
        int y_pos;

        if (i == 0) {
            y_pos = y_max;  // Bottom label (minimum value)
        } else if (i == count - 1) {
            y_pos = y_min;  // Top label (maximum value)
        } else {
            // Evenly distribute remaining labels between top and bottom boundaries
            float fraction = (float)i / (count - 1);
            y_pos = y_max - (int)((y_max - y_min) * fraction);
        }

        char buf[12];

        // Form a string with units only for the first label, others without units
        if (i == 0 && unit_start) {
            snprintf(buf, sizeof(buf), "%d%s", values[i], unit_start);
        } else {
            snprintf(buf, sizeof(buf), "%d", values[i]);
        }

        // For the left axis, use a fixed offset from the left (2 pixels)
        int x_pos = x;
        if (is_left_axis) {
            x_pos = 2;
        }

        // Output label on the screen with vertical offset -15 for visual alignment
        LCD_PutStr(x_pos, y_pos - 15, buf, FONT_tahoma_18X22, color, background);
    }
}

// Change graph axis label colors depending on sensor_values.current_state
void draw_axis_labels(void) {
    // Select label colors once depending on the state
    uint16_t color_temp_label = (sensor_values.current_state == RUN) ? color_actual : GRAPH_Color_TEMP;
    uint16_t color_power_label = (sensor_values.current_state == RUN) ? GRAPH_Color_POWER : B_BLUE;

    // Arrays of values for the temperature axis and the power axis
    int temp_values[] = {0, 90, 180, 270, 360, 450};
    int power_values[] = {0, 20, 40, 60, 80, 100};

    // Draw the left temperature axis (with "°C" symbol at the minimum value)
    draw_axis(temp_values, sizeof(temp_values)/sizeof(temp_values[0]), 2, true, color_temp_label, "°C", NULL);

    // Draw the right power axis (with % at the minimum value)
    draw_axis(power_values, sizeof(power_values)/sizeof(power_values[0]), GRAPH_X0 + GRAPH_WIDTH + 3, false, color_power_label, "%", "");
}


void check_and_update_labels(void) {
	uint8_t last_sensor_state = 0xFF;
	if (last_sensor_state != sensor_values.current_state) {
        draw_axis_labels();
        last_sensor_state = sensor_values.current_state;
    }
}

void draw_graph_update(void) {
    if (!initialized) {
        draw_graph_init();
    }

    check_and_update_labels();

    // Update temperature and power lines with partial erasing
    for (uint8_t i = 1; i < GRAPH_POINTS; i++) {
        uint8_t idx_prev = (index_graph + i - 1) % GRAPH_POINTS;
        uint8_t idx_curr = (index_graph + i) % GRAPH_POINTS;

        // Temperature points
        int x_prev_temp = GRAPH_X0 + ((i - 1) * GRAPH_WIDTH) / (GRAPH_POINTS - 1);
        int x_curr_temp = GRAPH_X0 + (i * GRAPH_WIDTH) / (GRAPH_POINTS - 1);
        int y_prev_temp = GRAPH_Y0 - ((temp_array[idx_prev] - TEMP_MIN) * GRAPH_HEIGHT) / (TEMP_MAX - TEMP_MIN);
        int y_curr_temp = GRAPH_Y0 - ((temp_array[idx_curr] - TEMP_MIN) * GRAPH_HEIGHT) / (TEMP_MAX - TEMP_MIN);

        uint16_t color_temp = (sensor_values.current_state == RUN) ? color_actual : GRAPH_Color_TEMP;

        Line new_temp_line = {x_prev_temp, y_prev_temp, x_curr_temp, y_curr_temp, color_temp};
        Line *old_temp_line = &temp_lines_prev[i - 1];

        // Power lines (offset +2 pixels)
        int x_prev_power = x_prev_temp;
        int x_curr_power = x_curr_temp;
        int y_prev_power = (GRAPH_Y0 - ((power_array[idx_prev] - POWER_MIN) * GRAPH_HEIGHT) / (POWER_MAX - POWER_MIN));
        int y_curr_power = (GRAPH_Y0 - ((power_array[idx_curr] - POWER_MIN) * GRAPH_HEIGHT) / (POWER_MAX - POWER_MIN));

        uint16_t color_power = (sensor_values.current_state == RUN) ? GRAPH_Color_POWER : B_BLUE;

        Line new_power_line = {x_prev_power, y_prev_power, x_curr_power, y_curr_power, color_power};
        Line *old_power_line = &power_lines_prev[i - 1];

        bool temp_line_changed = (old_temp_line->x1 != new_temp_line.x1) || (old_temp_line->y1 != new_temp_line.y1) ||
                                 (old_temp_line->x2 != new_temp_line.x2) || (old_temp_line->y2 != new_temp_line.y2) ||
                                 (old_temp_line->color != new_temp_line.color);

        bool power_line_changed = (old_power_line->x1 != new_power_line.x1) || (old_power_line->y1 != new_power_line.y1) ||
                                  (old_power_line->x2 != new_power_line.x2) || (old_power_line->y2 != new_power_line.y2) ||
                                  (old_power_line->color != new_power_line.color);

        // Check intersection of old lines if at least one line has changed
        if (temp_line_changed || power_line_changed) {
            bool overlap = false;

            if ((old_temp_line->x1 || old_temp_line->y1 || old_temp_line->x2 || old_temp_line->y2) &&
                (old_power_line->x1 || old_power_line->y1 || old_power_line->x2 || old_power_line->y2)) {
                overlap = lines_overlap(old_temp_line, old_power_line, 2);
            }

            if (overlap) {
                // Erase both lines and restore the grid
                erase_line_and_restore_grid(old_temp_line->x1, old_temp_line->y1, old_temp_line->x2, old_temp_line->y2);
                erase_line_and_restore_grid(old_power_line->x1, old_power_line->y1, old_power_line->x2, old_power_line->y2);
            } else {
                // Erase only changed lines individually
                if (temp_line_changed && (old_temp_line->x1 || old_temp_line->y1 || old_temp_line->x2 || old_temp_line->y2)) {
                    erase_line_and_restore_grid(old_temp_line->x1, old_temp_line->y1, old_temp_line->x2, old_temp_line->y2);
                }
                if (power_line_changed && (old_power_line->x1 || old_power_line->y1 || old_power_line->x2 || old_power_line->y2)) {
                    erase_line_and_restore_grid(old_power_line->x1, old_power_line->y1, old_power_line->x2, old_power_line->y2);
                }
            }
        }

        // Draw new lines
        draw_line_with_thickness(new_temp_line.x1, new_temp_line.y1, new_temp_line.x2, new_temp_line.y2, new_temp_line.color, 2);
        draw_line_with_thickness(new_power_line.x1, new_power_line.y1, new_power_line.x2, new_power_line.y2, new_power_line.color, 2);

        // Save new lines to prev
        temp_lines_prev[i - 1] = new_temp_line;
        power_lines_prev[i - 1] = new_power_line;
    }
}

