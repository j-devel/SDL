/*
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

/* Simple program to test the SDL game controller routines */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifndef SDL_JOYSTICK_DISABLED

#ifdef __IPHONEOS__
#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT    320
#else
#define SCREEN_WIDTH    512
#define SCREEN_HEIGHT   320
#endif

/* This is indexed by SDL_GameControllerButton. */
static const struct { int x; int y; } button_positions[] = {
    {387, 167},  /* A */
    {431, 132},  /* B */
    {342, 132},  /* X */
    {389, 101},  /* Y */
    {174, 132},  /* BACK */
    {233, 132},  /* GUIDE */
    {289, 132},  /* START */
    {75,  154},  /* LEFTSTICK */
    {305, 230},  /* RIGHTSTICK */
    {77,  40},   /* LEFTSHOULDER */
    {396, 36},   /* RIGHTSHOULDER */
    {154, 188},  /* DPAD_UP */
    {154, 249},  /* DPAD_DOWN */
    {116, 217},  /* DPAD_LEFT */
    {186, 217},  /* DPAD_RIGHT */
};

/* This is indexed by SDL_GameControllerAxis. */
static const struct { int x; int y; double angle; } axis_positions[] = {
    {74,  153, 270.0},  /* LEFTX */
    {74,  153, 0.0},  /* LEFTY */
    {306, 231, 270.0},  /* RIGHTX */
    {306, 231, 0.0},  /* RIGHTY */
    {91, -20, 0.0},     /* TRIGGERLEFT */
    {375, -20, 0.0},    /* TRIGGERRIGHT */
};

SDL_Window *window = NULL;
SDL_Renderer *screen = NULL;
SDL_bool retval = SDL_FALSE;
SDL_bool done = SDL_FALSE;
SDL_Texture *background, *button, *axis;
SDL_GameController *gamecontroller;

static SDL_Texture *
LoadTexture(SDL_Renderer *renderer, const char *file, SDL_bool transparent)
{
    SDL_Surface *temp = NULL;
    SDL_Texture *texture = NULL;

    temp = SDL_LoadBMP(file);
    if (temp == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s", file, SDL_GetError());
    } else {
        /* Set transparent pixel as the pixel at (0,0) */
        if (transparent) {
            if (temp->format->BytesPerPixel == 1) {
                SDL_SetColorKey(temp, SDL_TRUE, *(Uint8 *)temp->pixels);
            }
        }

        texture = SDL_CreateTextureFromSurface(renderer, temp);
        if (!texture) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture: %s\n", SDL_GetError());
        }
    }
    if (temp) {
        SDL_FreeSurface(temp);
    }
    return texture;
}

static void
UpdateWindowTitle()
{
    const char *name = SDL_GameControllerName(gamecontroller);
    const char *basetitle = "Game Controller Test: ";
    const size_t titlelen = SDL_strlen(basetitle) + SDL_strlen(name) + 1;
    char *title = (char *)SDL_malloc(titlelen);

    retval = SDL_FALSE;
    done = SDL_FALSE;

    if (title) {
        SDL_snprintf(title, titlelen, "%s%s", basetitle, name);
        SDL_SetWindowTitle(window, title);
        SDL_free(title);
    }
}

void
loop(void *arg)
{
    SDL_Event event;
    int i;

    /* blank screen, set up for drawing this frame. */
    SDL_SetRenderDrawColor(screen, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(screen);
    SDL_RenderCopy(screen, background, NULL, NULL);

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_Log("Game controller device %d added.\n", (int) event.cdevice.which);
            if (!gamecontroller) {
                gamecontroller = SDL_GameControllerOpen(event.cdevice.which);
                if (gamecontroller) {
                    UpdateWindowTitle();
                } else {
                    SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
                }
            }
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            SDL_Log("Game controller device %d removed.\n", (int) event.cdevice.which);
            if (gamecontroller && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamecontroller))) {
                SDL_GameControllerClose(gamecontroller);
                gamecontroller = SDL_GameControllerOpen(0);
                if (gamecontroller) {
                    UpdateWindowTitle();
                }
            }
            break;

        case SDL_CONTROLLERAXISMOTION:
            SDL_Log("Controller axis %s changed to %d\n", SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis), event.caxis.value);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            SDL_Log("Controller button %s %s\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state ? "pressed" : "released");
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym != SDLK_ESCAPE) {
                break;
            }
            /* Fall through to signal quit */
        case SDL_QUIT:
            done = SDL_TRUE;
            break;
        default:
            break;
        }
    }

    if (gamecontroller) {
        /* Update visual controller state */
        for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            if (SDL_GameControllerGetButton(gamecontroller, (SDL_GameControllerButton)i) == SDL_PRESSED) {
                const SDL_Rect dst = { button_positions[i].x, button_positions[i].y, 50, 50 };
                SDL_RenderCopyEx(screen, button, NULL, &dst, 0, NULL, SDL_FLIP_NONE);
            }
        }

        for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
            const Sint16 deadzone = 8000;  /* !!! FIXME: real deadzone */
            const Sint16 value = SDL_GameControllerGetAxis(gamecontroller, (SDL_GameControllerAxis)(i));
            if (value < -deadzone) {
                const SDL_Rect dst = { axis_positions[i].x, axis_positions[i].y, 50, 50 };
                const double angle = axis_positions[i].angle;
                SDL_RenderCopyEx(screen, axis, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
            } else if (value > deadzone) {
                const SDL_Rect dst = { axis_positions[i].x, axis_positions[i].y, 50, 50 };
                const double angle = axis_positions[i].angle + 180.0;
                SDL_RenderCopyEx(screen, axis, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
            }
        }

        /* Update rumble based on trigger state */
        {
            Uint16 low_frequency_rumble = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) * 2;
            Uint16 high_frequency_rumble = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) * 2;
            SDL_GameControllerRumble(gamecontroller, low_frequency_rumble, high_frequency_rumble, 250);
        }
    }

    SDL_RenderPresent(screen);

#ifdef __EMSCRIPTEN__
    if (done) {
        emscripten_cancel_main_loop();
    }
#endif
}

int
main(int argc, char *argv[])
{
    int i;
    int nController = 0;
    char guid[64];

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    /* Initialize SDL (Note: video is required to start event loop) */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    /* Print information about the mappings */
    if (argv[1] && SDL_strcmp(argv[1], "--mappings") == 0) {
        SDL_Log("Supported mappings:\n");
        for (i = 0; i < SDL_GameControllerNumMappings(); ++i) {
            char *mapping = SDL_GameControllerMappingForIndex(i);
            if (mapping) {
                SDL_Log("\t%s\n", mapping);
                SDL_free(mapping);
            }
        }
        SDL_Log("\n");
    }

    /* Print information about the controller */
    for (i = 0; i < SDL_NumJoysticks(); ++i) {
        const char *name;
        const char *description;

        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),
                                  guid, sizeof (guid));

        if ( SDL_IsGameController(i) ) {
            nController++;
            name = SDL_GameControllerNameForIndex(i);
            switch (SDL_GameControllerTypeForIndex(i)) {
            case SDL_CONTROLLER_TYPE_XBOX360:
                description = "XBox 360 Controller";
                break;
            case SDL_CONTROLLER_TYPE_XBOXONE:
                description = "XBox One Controller";
                break;
            case SDL_CONTROLLER_TYPE_PS3:
                description = "PS3 Controller";
                break;
            case SDL_CONTROLLER_TYPE_PS4:
                description = "PS4 Controller";
                break;
            case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
                description = "Nintendo Switch Pro Controller";
                break;
            case SDL_CONTROLLER_TYPE_VIRTUAL:
                description = "Virtual Game Controller";
                break;
            default:
                description = "Game Controller";
                break;
            }
        } else {
            name = SDL_JoystickNameForIndex(i);
            description = "Joystick";
        }
        SDL_Log("%s %d: %s (guid %s, VID 0x%.4x, PID 0x%.4x, player index = %d)\n",
            description, i, name ? name : "Unknown", guid,
            SDL_JoystickGetDeviceVendor(i), SDL_JoystickGetDeviceProduct(i), SDL_JoystickGetDevicePlayerIndex(i));
    }
    SDL_Log("There are %d game controller(s) attached (%d joystick(s))\n", nController, SDL_NumJoysticks());

    /* Create a window to display controller state */
    window = SDL_CreateWindow("Game Controller Test", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, 0);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s\n", SDL_GetError());
        return 2;
    }

    screen = SDL_CreateRenderer(window, -1, 0);
    if (screen == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 2;
    }

    SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(screen);
    SDL_RenderPresent(screen);

    /* scale for platforms that don't give you the window size you asked for. */
    SDL_RenderSetLogicalSize(screen, SCREEN_WIDTH, SCREEN_HEIGHT);

    background = LoadTexture(screen, "controllermap.bmp", SDL_FALSE);
    button = LoadTexture(screen, "button.bmp", SDL_TRUE);
    axis = LoadTexture(screen, "axis.bmp", SDL_TRUE);

    if (!background || !button || !axis) {
        SDL_DestroyRenderer(screen);
        SDL_DestroyWindow(window);
        return 2;
    }
    SDL_SetTextureColorMod(button, 10, 255, 21);
    SDL_SetTextureColorMod(axis, 10, 255, 21);

    /* !!! FIXME: */
    /*SDL_RenderSetLogicalSize(screen, background->w, background->h);*/

    /* Loop, getting controller events! */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(loop, NULL, 0, 1);
#else
    while (!done) {
        loop(NULL);
    }
#endif

    SDL_DestroyRenderer(screen);
    screen = NULL;
    background = NULL;
    button = NULL;
    axis = NULL;
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

    return 0;
}

#else

int
main(int argc, char *argv[])
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL compiled without Joystick support.\n");
    return 1;
}

#endif

/* vi: set ts=4 sw=4 expandtab: */
