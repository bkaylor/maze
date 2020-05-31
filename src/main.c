#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "arena.h"
#include "draw.h"
#include "button.h"
#include "queue.h"

typedef struct {
    int x;
    int y;
} Window;

typedef struct {
    int x;
    int y;
    int id;

    bool visited;
    bool north_wall, east_wall, south_wall, west_wall;

    void *came_from; // Compiler complains if this is a struct Cell *.
    bool on_solution_path;
    bool checked;
} Cell;

typedef struct {
    Cell *start;
    Cell *end;
    bool solved;
} Solution_Info;

typedef struct {
    int rows;
    int columns;

    Solution_Info solution_info;

    SDL_Rect rect;

    Cell *cells;
} Grid;

typedef struct {
    Window window;

    Grid grid;
    int rows;
    int columns;
    int max_cells;
    Arena arena;

    bool show_options;

    Gui gui;

    Mouse_Info mouse_info;

    bool quit;
    bool reset;
} State;

void render(SDL_Renderer *renderer, State state)
{
    SDL_RenderClear(renderer);

    // Set background color.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    // Draw the maze.
    Grid g = state.grid;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    float width = (float)g.rect.w/(float)g.columns;
    float height = (float)g.rect.h/(float)g.rows;

    float side_length = width > height ? height : width;

    for (int i = 0; i < g.columns; i += 1)
    {
        for (int j = 0; j < g.rows; j += 1)
        {
            Cell c = g.cells[(j*g.columns) + i];

            if (c.north_wall) {
                SDL_RenderDrawLine(renderer, 
                                   g.rect.x + c.x*side_length, 
                                   g.rect.y + c.y*side_length, 
                                   g.rect.x + (c.x+1)*side_length, 
                                   g.rect.y + c.y*side_length);
            }

            if (c.east_wall) {
                SDL_RenderDrawLine(renderer, 
                                   g.rect.x + (c.x+1)*side_length, 
                                   g.rect.y + c.y*side_length, 
                                   g.rect.x + (c.x+1)*side_length, 
                                   g.rect.y + (c.y+1)*side_length);
            }

            if (c.south_wall) {
                SDL_RenderDrawLine(renderer, 
                                   g.rect.x + c.x*side_length, 
                                   g.rect.y + (c.y+1)*side_length, 
                                   g.rect.x + (c.x+1)*side_length, 
                                   g.rect.y + (c.y+1)*side_length);
            }

            if (c.west_wall) {
                SDL_RenderDrawLine(renderer, 
                                   g.rect.x + c.x*side_length, 
                                   g.rect.y + c.y*side_length, 
                                   g.rect.x + c.x*side_length, 
                                   g.rect.y + (c.y+1)*side_length);
            }

            /*
            if (c.on_solution_path) {
                SDL_Rect r;
                r.x = g.rect.x + c.x*side_length + 2;
                r.y = g.rect.y + c.y*side_length + 2;
                r.w = side_length - 2;
                r.h = side_length - 2;

                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            */
        }
    }

    if (g.solution_info.solved)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

        Cell *at = g.solution_info.end;
        while (at->id != g.solution_info.start->id)
        {
            Cell *came_from = at->came_from;
            SDL_RenderDrawLine(renderer, 
                               g.rect.x + ((at->x + 0.5f)*side_length),
                               g.rect.y + ((at->y + 0.5f)*side_length),
                               g.rect.x + ((came_from->x + 0.5f)*side_length),
                               g.rect.y + ((came_from->y + 0.5f)*side_length));

            at = came_from;
        }
    }

    draw_all_buttons(renderer, &state.gui);

    SDL_RenderPresent(renderer);
}

Cell *get_cell_at(Grid *g, int x, int y)
{
    Cell *result = NULL;

    if ((0 <= x && x < g->columns) &&
        (0 <= y && y < g->rows))
    {
        result = &g->cells[(g->columns*y) + x];
    }

    return result;
}

Cell *get_north(Grid *g, Cell *c)
{
    Cell *result = get_cell_at(g, c->x, c->y-1);
    return result;
}

Cell *get_east(Grid *g, Cell *c)
{
    Cell *result = get_cell_at(g, c->x+1, c->y);
    return result;
}

Cell *get_south(Grid *g, Cell *c)
{
    Cell *result = get_cell_at(g, c->x, c->y+1);
    return result;
}

Cell *get_west(Grid *g, Cell *c)
{
    Cell *result = get_cell_at(g, c->x-1, c->y);
    return result;
}

void reset_solve_data(Grid *g)
{
    for (int i = 0; i < g->rows*g->columns; i += 1)
    {
        Cell *c = &g->cells[i];
        c->checked = false;
        c->on_solution_path = false;
    }
}

bool breadth(Grid *g, Cell *start, Cell *end)
{
    g->solution_info.start = start;
    g->solution_info.end = end;

    Queue q;
    queue_init(&q, g->rows*g->columns);

    queue_enqueue(&q, start->id);

    while (q.item_count > 0)
    {
        int id = queue_dequeue(&q);

        Cell *c = &g->cells[id];

        Cell *n = get_north(g, c);
        Cell *e = get_east(g, c);
        Cell *s = get_south(g, c);
        Cell *w = get_west(g, c);

        if (n != NULL && !c->north_wall && !n->checked) 
        {
            queue_enqueue(&q, n->id);
            n->checked = true;
            n->came_from = c;
        }

        if (e != NULL && !c->east_wall && !e->checked) 
        {
            queue_enqueue(&q, e->id);
            e->checked = true;
            e->came_from = c;
        }

        if (s != NULL && !c->south_wall && !s->checked) 
        {
            queue_enqueue(&q, s->id);
            s->checked = true;
            s->came_from = c;
        }

        if (w != NULL && !c->west_wall && !w->checked) 
        {
            queue_enqueue(&q, w->id);
            w->checked = true;
            w->came_from = c;
        }
    }

    Cell *c = end;
    while (true)
    {
        c->on_solution_path = true;
        if (c->id == start->id) return true; 
        c = c->came_from;
    }

    return false;
}

bool depth(Grid *g, Cell *c)
{
    c->checked = true;

    if (c->x == g->columns-1 && c->y == g->rows-1) 
    {
        c->on_solution_path = true;
        return true;
    }

    Cell *n = get_north(g, c);
    Cell *e = get_east(g, c);
    Cell *s = get_south(g, c);
    Cell *w = get_west(g, c);

    if (n != NULL && !c->north_wall && !n->checked) 
    {
        if (depth(g, n))
        {
            n->came_from = c;
            c->on_solution_path = true;
            return true;
        }
    }

    if (e != NULL && !c->east_wall && !e->checked) 
    {
        if (depth(g, e))
        {
            e->came_from = c;
            c->on_solution_path = true;
            return true;
        }
    }

    if (s != NULL && !c->south_wall && !s->checked) 
    {
        if (depth(g, s))
        {
            s->came_from = c;
            c->on_solution_path = true;
            return true;
        }
    }

    if (w != NULL && !c->west_wall && !w->checked) 
    {
        if (depth(g, w))
        {
            w->came_from = c;
            c->on_solution_path = true;
            return true;
        }
    }

    return false;
}

void visit(Grid *g, Cell *c)
{
    c->visited = true;

    Cell *n = get_north(g, c);
    Cell *e = get_east(g, c);
    Cell *s = get_south(g, c);
    Cell *w = get_west(g, c);

    int unvisited_neighbor_count;
    while (true)
    {
        unvisited_neighbor_count = 0;

        if (n != NULL && !n->visited) {
            unvisited_neighbor_count += 1;
        }
        if (s != NULL && !s->visited) {
            unvisited_neighbor_count += 1;
        }
        if (e != NULL && !e->visited) {
            unvisited_neighbor_count += 1;
        }
        if (w != NULL && !w->visited) {
            unvisited_neighbor_count += 1;
        }

        if (unvisited_neighbor_count == 0) return;

        int which_neighbor = rand() % unvisited_neighbor_count;

        if (n != NULL && !n->visited) {
            which_neighbor -= 1;
            if (which_neighbor == -1) {
                c->north_wall = false;
                n->south_wall = false;
                visit(g, n);
                unvisited_neighbor_count -= 1;
                continue;
            }
        }
        if (s != NULL && !s->visited) {
            which_neighbor -= 1;
            if (which_neighbor == -1) {
                c->south_wall = false;
                s->north_wall = false;
                visit(g, s);
                unvisited_neighbor_count -= 1;
                continue;
            }
        }
        if (e != NULL && !e->visited) {
            which_neighbor -= 1;
            if (which_neighbor == -1) {
                c->east_wall = false;
                e->west_wall = false;
                visit(g, e);
                unvisited_neighbor_count -= 1;
                continue;
            }
        }
        if (w != NULL && !w->visited) {
            which_neighbor -= 1;
            if (which_neighbor == -1) {
                c->west_wall = false;
                w->east_wall = false;
                visit(g, w);
                unvisited_neighbor_count -= 1;
                continue;
            }
        }
    }
}

void generate_maze(Grid *g)
{
    for (int i = 0; i < g->columns; i += 1)
    {
        for (int j = 0; j < g->rows; j += 1)
        {
            Cell c;
            c.x = i;
            c.y = j;
            c.visited = false;
            c.north_wall = true;
            c.east_wall = true;
            c.south_wall = true;
            c.west_wall = true;
            c.on_solution_path = false;
            c.checked = false;
            c.id = (j*g->columns) + i;

            g->cells[c.id] = c;
        }
    }

    Cell *c = &g->cells[0];

    visit(g, c);
}

void handle_buttons(State *state)
{
    Gui *g = &state->gui;

    gui_frame_init(g);
    g->mouse_info = state->mouse_info;

    new_button_stack(g, 5, 5, 5);

    if (do_button_tiny(g, "?"))
    {
        state->show_options = !state->show_options;
    }

    if (state->show_options)
    {
        if (do_button(g, "row +"))
        {
            state->rows += 1;

            if ((state->rows * state->columns) >= state->max_cells) {
                state->rows -= 1;
            } else {
                state->reset = true;
            }
        }

        if (do_button(g, "row -"))
        {
            state->rows -= 1;
            if (state->rows < 1) {
                state->rows = 1;
            } else {
                state->reset = true;
            }
        }

        if (do_button(g, "column +"))
        {
            state->columns += 1;

            if ((state->columns * state->rows) >= state->max_cells) {
                state->columns -= 1;
            } else {
                state->reset = true;
            }
        }

        if (do_button(g, "column -"))
        {
            state->columns -= 1;
            if (state->columns < 1) {
                state->columns = 1;
            } else {
                state->reset = true;
            }
        }

        if (do_button(g, "depth"))
        {
            reset_solve_data(&state->grid);

            Solution_Info *si = &state->grid.solution_info;
            si->solved = depth(&state->grid, &state->grid.cells[0]);
        }

        if (do_button(g, "breadth"))
        {
            reset_solve_data(&state->grid);

            Solution_Info *si = &state->grid.solution_info;
            si->solved = breadth(&state->grid, si->start, si->end);
        }

        if (do_button(g, "reset"))
        {
            state->reset = true;
        }

        if (do_button(g, "quit"))
        {
            state->quit = true;
        }
    }
}

void update(State *state) 
{
    if (state->reset)
    {
        arena_free_all(&state->arena);

        Grid *g = &state->grid;

        g->rows = state->rows;
        g->columns = state->columns;
        g->cells = arena_alloc(&state->arena, sizeof(Cell) * g->rows * g->columns);

        float percent_padding = 0.10f;

        SDL_Rect r;
        r.x = state->window.x * percent_padding;
        r.y = state->window.y * percent_padding;
        r.w = (state->window.x * (1.0f - (2*percent_padding)));
        r.h = (state->window.y * (1.0f - (2*percent_padding)));

        g->rect = r;

        generate_maze(g);

        g->solution_info.start = &g->cells[0]; 
        g->solution_info.end = &g->cells[g->columns*g->rows-1]; 
        g->solution_info.solved = false;

        state->reset = false;
    }

    handle_buttons(state);

    return;
}

void get_input(State *state)
{
    SDL_GetMouseState(&state->mouse_info.x, &state->mouse_info.y);
    state->mouse_info.clicked = false;

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        state->show_options = !state->show_options;
                        break;
                        
                    case SDLK_r:
                        state->reset = true;
                        break;

                    case SDLK_q:
                        if (state->show_options) {
                            state->quit = true;
                        }
                        break;

                    default:
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    state->mouse_info.clicked = true;
                }
                break;

            case SDL_QUIT:
                state->quit = true;
                break;

            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init video error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init audio error: %s\n", SDL_GetError());
        return 1;
    }

    // SDL_ShowCursor(SDL_DISABLE);

	// Setup window
	SDL_Window *win = SDL_CreateWindow("Maze",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			1440, 980,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Setup renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Setup font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("liberation.ttf", 12);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), win);
		return -666;
	}

    // Setup main loop
    srand(time(NULL));

    State state;
    state.quit = false;
    state.reset = true;
    state.show_options = false;
    state.rows = 20;
    state.columns = 20;
    state.max_cells = 10000;

    gui_init(&state.gui, font);
    
    arena_init(&state.arena, malloc(sizeof(Cell) * state.max_cells), sizeof(Cell) * state.max_cells);

    while (!state.quit)
    {
        SDL_PumpEvents();
        get_input(&state);

        if (!state.quit)
        {
            int x, y;
            SDL_GetWindowSize(win, &state.window.x, &state.window.y);

            update(&state);
            render(ren, state);
        }
    }

    return 0;
}
