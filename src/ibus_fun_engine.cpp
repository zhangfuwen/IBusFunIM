//
// Created by zhangfuwen on 2022/1/27.
//

#include "ibus_fun_engine.h"
#include "Engine.h"
#include "common_log.h"

// ------------------- define type ---------------------
#define IBUS_FUN_ENGINE(obj)             \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_FUN_ENGINE, IBusFunEngine))
#define IBUS_FUN_ENGINE_CLASS(klass)     \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_FUN_ENGINE, IBusFunEngineClass))
#define IBUS_IS_FUN_ENGINE(obj)          \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_FUN_ENGINE))
#define IBUS_IS_FUN_ENGINE_CLASS(klass)  \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_FUN_ENGINE))
#define IBUS_FUN_ENGINE_GET_CLASS(obj)   \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_FUN_ENGINE, IBusFunEngineClass))

typedef struct _IBusFunEngine IBusFunEngine;
typedef struct _IBusFunEngineClass IBusFunEngineClass;

struct _IBusFunEngine {
    IBusEngine parent;

    /* members */
    Engine * funEngine;
};

struct _IBusFunEngineClass {
    IBusEngineClass  parent;
};

G_DEFINE_TYPE(IBusFunEngine, ibus_fun_engine, IBUS_TYPE_ENGINE)

static void ibus_fun_engine_init(IBusFunEngine *engine) {
    FUN_DEBUG("Entry");
    if (g_object_is_floating(engine)) {
        g_object_ref_sink(engine);
    }
    FUN_DEBUG("Exit");
}

// ------------------- ibus extended ``````````````````````````````````````

static GObject* ibus_fun_engine_constructor  (GType                   type,
                                              guint                   n_construct_params,
                                              GObjectConstructParam  *construct_param);
static void ibus_fun_engine_destroy(IBusFunEngine * fun_engine);
static gboolean
ibus_fun_engine_process_key_event
    (IBusEngine             *engine,
     guint               	 keyval,
     guint               	 keycode,
     guint               	 modifiers);
static void ibus_fun_engine_focus_in    (IBusEngine             *engine);
static void ibus_fun_engine_focus_out   (IBusEngine             *engine);
static void ibus_fun_engine_reset       (IBusEngine             *engine);
static void ibus_fun_engine_enable      (IBusEngine             *engine);
static void ibus_fun_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_fun_engine_set_capabilities
    (IBusEngine             *engine,
     guint                   caps);
static void ibus_fun_engine_page_up     (IBusEngine             *engine);
static void ibus_fun_engine_page_down   (IBusEngine             *engine);
static void ibus_fun_engine_cursor_up   (IBusEngine             *engine);
static void ibus_fun_engine_cursor_down (IBusEngine             *engine);
static void ibus_fun_engine_property_activate  (IBusEngine             *engine,
                                         const gchar            *prop_name,
                                         guint                    prop_state);
static void     ibus_fun_engine_candidate_clicked
    (IBusEngine             *engine,
     guint                   index,
     guint                   button,
     guint                   state);
static void ibus_fun_engine_property_show
    (IBusEngine             *engine,
     const gchar            *prop_name);
static void ibus_fun_engine_property_hide
    (IBusEngine             *engine,
     const gchar            *prop_name);

static void ibus_fun_engine_commit_string
    (IBusFunEngine      *fun,
     const gchar            *string);
static void ibus_fun_engine_update      (IBusFunEngine      *fun);

static void ibus_fun_engine_class_init(IBusFunEngineClass *klass) {
    FUN_DEBUG("Entry");
    GObjectClass * object_class = G_OBJECT_CLASS (klass);
    IBusObjectClass * ibus_object_class = IBUS_OBJECT_CLASS(klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);

    object_class->constructor = ibus_fun_engine_constructor;
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_fun_engine_destroy;

    engine_class->process_key_event = ibus_fun_engine_process_key_event;

    engine_class->reset = ibus_fun_engine_reset;
    engine_class->enable = ibus_fun_engine_enable;
    engine_class->disable = ibus_fun_engine_disable;

    engine_class->focus_in = ibus_fun_engine_focus_in;
    engine_class->focus_out = ibus_fun_engine_focus_out;

    engine_class->page_up = ibus_fun_engine_page_up;
    engine_class->page_down = ibus_fun_engine_page_down;

    engine_class->cursor_up = ibus_fun_engine_cursor_up;
    engine_class->cursor_down = ibus_fun_engine_cursor_down;

    engine_class->property_activate = ibus_fun_engine_property_activate;

    engine_class->candidate_clicked = ibus_fun_engine_candidate_clicked;
    FUN_DEBUG("Exit");
}

static GObject*
ibus_fun_engine_constructor (GType                  type,
                               guint                  n_construct_params,
                               GObjectConstructParam *construct_params)
{
    FUN_DEBUG("Entry");
    IBusFunEngine *engine;
    const gchar *name;

    engine = (IBusFunEngine *) G_OBJECT_CLASS (ibus_fun_engine_parent_class)->constructor (
        type,
        n_construct_params,
        construct_params);

    engine->funEngine = new Engine(IBUS_ENGINE (engine));
    FUN_DEBUG("Exit");
    return (GObject *) engine;
}

static void
ibus_fun_engine_destroy (IBusFunEngine *ibus_fun_engine)
{
    FUN_DEBUG("Entry");
    delete ibus_fun_engine->funEngine;
    ((IBusObjectClass *) ibus_fun_engine_parent_class)->destroy ((IBusObject *)ibus_fun_engine);
    FUN_DEBUG("Exit");
}

static gboolean
ibus_fun_engine_process_key_event (IBusEngine     *engine,
                                     guint           keyval,
                                     guint           keycode,
                                     guint           modifiers)
{
    IBusFunEngine *fun = (IBusFunEngine *) engine;
    return fun->funEngine->ProcessKeyEvent (keyval, keycode, modifiers);
}

static void
ibus_fun_engine_property_activate (IBusEngine    *engine,
                                     const gchar   *prop_name,
                                     guint          prop_state)
{
    IBusFunEngine *fun = (IBusFunEngine *) engine;
    fun->funEngine->OnPropertyActivate (engine, prop_name, prop_state);
}
static void
ibus_fun_engine_candidate_clicked (IBusEngine *engine,
                                     guint       index,
                                     guint       button,
                                     guint       state)
{
    IBusFunEngine *fun = (IBusFunEngine *) engine;
    fun->funEngine->OnCandidateClicked (engine, index, button, state);
}

#define FUNCTION(name, Name)                                        \
    static void                                                     \
    ibus_fun_engine_##name (IBusEngine *engine)                  \
    {                                                               \
        IBusFunEngine *ibus_fun_engine = (IBusFunEngine *) engine;     \
        ibus_fun_engine->funEngine->Name ();                                    \
        ((IBusEngineClass *) ibus_fun_engine_parent_class)       \
            ->name (engine);                                        \
    }
FUNCTION(focus_in,    FocusIn)
FUNCTION(focus_out,   FocusOut)
FUNCTION(reset,       Reset)
FUNCTION(enable,      Enable)
FUNCTION(disable,     Disable)
FUNCTION(page_up,     PageUp)
FUNCTION(page_down,   PageDown)
FUNCTION(cursor_up,   CursorUp)
FUNCTION(cursor_down, CursorDown)
#undef FUNCTION