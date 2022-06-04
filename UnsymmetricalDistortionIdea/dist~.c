#include "m_pd.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* ------------------------ dist~ ----------------------------- */
/*
    This is an extension of the classic hard clip distortion effect.
    Instead of clipping at -1 and 1 and applying global gain.
    This creates a more bespoke clip distortion effect.
 
    In this case, I'm just giving it a hard max and min value, but a better approach
    is to apply gain to positive/negative values seperately. I'm not conviced this
    sounds better. It sounds different...
 
*/
static t_class *dist_class;

typedef struct _dist
{
    t_object x_obj;
    t_float x_f;
    
    //signal max and min
    t_float sigMin;
    t_float sigMax;
    
    t_float DCval;
    
} t_dist;

//Generic offset
void CalcDC (t_dist *x)
{
    x->DCval = ((x->sigMin * -1) - x->sigMax) * 0.5;
    post("%f",x->DCval);
}


void dist_float(t_dist *x, t_floatarg f)
{
    
    if(f < -1 || f > 1)
    {
        post("Please keep the values between -1 and 1.");
        return;
    }else
        x->sigMin = f;
    
    CalcDC(x);
}

void dist_in3(t_dist *x, t_floatarg g)
{
    
    if(g < -1 || g > 1)
    {
        post("Please keep the values between -1 and 1.");
        return;
    }else
        x->sigMax = g;
    
    CalcDC(x);
}

static t_int *dist_perform(t_int *w)
{
    t_dist *x = (t_dist *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);
    
    float MaxVal = x->sigMax;
    float MinVal = x->sigMin;
    float DC = x->DCval;
    
    //scale the signal level to be more audible
    float ampScal = 1 / ((MaxVal - MinVal)/2);
    
    while (n--)
    {
        float f = *(in++);
        
        //clipping samples based on the min and max values
        if (f < MinVal)
            f = MinVal;
        
        if(f > MaxVal)
            f = MaxVal;

        
        *out++ = (f + DC) * ampScal;
    }
    return (w+5);
}


static void dist_dsp(t_dist *x, t_signal **sp)
{
    dsp_add(dist_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void *dist_new(void)
{
    t_dist *x = (t_dist *)pd_new(dist_class);
    
    //create inlets
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("float"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"),gensym("in3"));
    
    //create outlet
    outlet_new(&x->x_obj, gensym("signal"));
    
    //set basic values
    x->x_f = 0;
    x->sigMin = -1;
    x->sigMax = 1;
    
    return (x);
}

void dist_tilde_setup(void)
{
    dist_class = class_new(gensym("dist~"), (t_newmethod)dist_new, 0,
        sizeof(t_dist), 0, A_DEFFLOAT, 0);

    CLASS_MAINSIGNALIN(dist_class, t_dist, x_f);

    class_addmethod(dist_class, (t_method)dist_dsp, gensym("dsp"), 0);
    class_addmethod(dist_class, (t_method)dist_float, gensym("float"), A_FLOAT, 0);
    class_addmethod(dist_class, (t_method)dist_in3, gensym("in3"), A_FLOAT, 0);
}
