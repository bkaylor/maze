
typedef struct {
    int x, y;
    bool clicked;
} Mouse_Info;

typedef struct {
    int width;
    int height;
} Button_Size;

typedef struct {
    SDL_Color text_color;
    SDL_Color color;

    TTF_Font *font;
} Button_Style;

typedef struct {
    SDL_Rect rect;
    char *text;
    Button_Style style;

    bool _hovered;
    bool _pressed;
} Button;

typedef struct {
    Mouse_Info mouse_info;

    Button_Style style;
    Button_Size size;

    int padding_between_buttons;
    int indent;

    Button buttons_to_render[50];
    int button_count;

    int stack_x;
    int stack_y;
    int stack_count;
} Gui;

// Call once in your program.
void gui_init(Gui *g, TTF_Font *font)
{
    g->size = (Button_Size){85, 30};
    g->style = (Button_Style){
        (SDL_Color){255, 255, 255, 255},
        (SDL_Color){50, 50, 50, 255},
        font
    };
}

// Each frame, call this to reset some per-frame data.
// Note that the user has to set mouse_position and clicking themselves.
// A button stack is started for them by default at 0, 0, if you just want to start calling
// do_button.
void gui_frame_init(Gui *g)
{
    g->mouse_info = (Mouse_Info){0, 0, false};

    g->button_count = 0;

    g->stack_x = 0;
    g->stack_y = 0;
    g->stack_count = 0;
    g->padding_between_buttons = 5;
}

// Start a new button stack at a given location.
void new_button_stack(Gui *g, int x, int y, int padding)
{
    g->stack_x = x;
    g->stack_y = y;
    g->stack_count = 0;
    g->padding_between_buttons = padding;
}

bool do_button_tiny(Gui *g, char *text)
{
    Button b;
    int x = 0;
    int y = 0;

    if (g->stack_count == 0) {
        x = g->stack_x;
        y = g->stack_y;
    } else {
        x = g->stack_x;
        y = g->stack_y + (g->stack_count * (g->size.height + g->padding_between_buttons));
    }

    b.rect = (SDL_Rect){x, y, 15, 15};
    b.text = text;
    b.style = g->style;

    b._hovered =  ((g->mouse_info.x >= b.rect.x) && 
        (g->mouse_info.x <= b.rect.x + b.rect.w) &&
        (g->mouse_info.y >= b.rect.y) && 
        (g->mouse_info.y <= b.rect.y + b.rect.h));

    b._pressed = b._hovered && g->mouse_info.clicked;

    g->buttons_to_render[g->button_count] = b;

    g->button_count += 1;
    g->stack_count += 1;

    return b._pressed;
}

bool do_button(Gui *g, char *text)
{
    Button b;
    int x = 0;
    int y = 0;

    if (g->stack_count == 0) {
        x = g->stack_x;
        y = g->stack_y;
    } else {
        x = g->stack_x;
        y = g->stack_y + (g->stack_count * (g->size.height + g->padding_between_buttons));
    }

    b.rect = (SDL_Rect){x, y, g->size.width, g->size.height};
    b.text = text;
    b.style = g->style;

    b._hovered =  ((g->mouse_info.x >= b.rect.x) && 
        (g->mouse_info.x <= b.rect.x + b.rect.w) &&
        (g->mouse_info.y >= b.rect.y) && 
        (g->mouse_info.y <= b.rect.y + b.rect.h));

    b._pressed = b._hovered && g->mouse_info.clicked;

    g->buttons_to_render[g->button_count] = b;

    g->button_count += 1;
    g->stack_count += 1;

    return b._pressed;
}

void draw_button(SDL_Renderer *renderer, Button button)
{
    SDL_SetRenderDrawColor(renderer, 
                           button.style.color.r * 0.7, 
                           button.style.color.g * 0.7, 
                           button.style.color.b * 0.7, 
                           button.style.color.a);

    SDL_Rect shadow_rect = {
        button.rect.x + button.rect.w * 0.04,
        button.rect.y + button.rect.w * 0.04,
        button.rect.w,
        button.rect.h
    };

    SDL_RenderFillRect(renderer, &shadow_rect);

    if (button._hovered) {
        SDL_SetRenderDrawColor(renderer, 
                               button.style.color.r * 1.5, 
                               button.style.color.g * 1.5, 
                               button.style.color.b * 1.5, 
                               button.style.color.a);
    } else {
        SDL_SetRenderDrawColor(renderer, 
                               button.style.color.r, 
                               button.style.color.g, 
                               button.style.color.b, 
                               button.style.color.a);
    }

    SDL_RenderFillRect(renderer, &button.rect);
    draw_centered_text(renderer, 
              button.rect, 
              button.text,
              button.style.font,
              button.style.text_color);
}

void draw_all_buttons(SDL_Renderer *renderer, Gui *g)
{
    for (int i = 0; i < g->button_count; i += 1)
    {
        Button b = g->buttons_to_render[i];
        draw_button(renderer, b);
    }

    g->button_count = 0;
}

