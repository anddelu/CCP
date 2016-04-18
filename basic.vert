const float PI = 3.14159265358979323846264;
const float THETA = 15.0 * 3.14159265358979323846264/180.0;

// Cross product of v1 and v2
float cross(in vec2 v1, in vec2 v2)
{
    return v1.x*v2.y - v1.y*v2.x;
}

// ----------------------------------------------------------------------------
// Returns distance of v3 to line v1-v2
float signed_distance(in vec2 v1, in vec2 v2, in vec2 v3)
{
    return cross(v2-v1,v1-v3) / length(v2-v1);
}


// Uniforms
// ------------------------------------
uniform mat4      u_mvpMatrix;
uniform vec4 u_color;
uniform float u_lineWidth;
uniform float u_antialias;
uniform vec2 u_lineCaps;
uniform float u_linejoin;
uniform float u_miter_limit;
uniform float u_length;
uniform float u_closed;

// Attributes
// ------------------------------------
attribute vec2 a_position;
attribute vec2 a_segment;
attribute vec2 a_angles;
attribute vec4 a_tangents;
attribute vec2 a_texcoord;
//attribute float a_index;

// Varying
// ------------------------------------
varying vec4  v_color;
varying vec2  v_segment;
varying vec2  v_angles;
varying vec2  v_linecaps;
varying vec2  v_texcoord;
varying vec2  v_miter;
varying float v_miter_limit;
varying float v_length;
varying float v_linejoin;
varying float v_linewidth;
varying float v_antialias;
varying float v_closed;
void main()
{
	v_color = u_color;

    v_linewidth = u_lineWidth;
    v_antialias = u_antialias;
    v_linecaps  = u_lineCaps;

   v_linejoin    = u_linejoin;
    v_miter_limit = u_miter_limit;
    v_length      = u_length;
    v_closed      = u_closed;

    bool closed = (v_closed > 0.0);

    v_angles  = a_angles;
    v_segment = a_segment;

    // Thickness below 1 pixel are represented using a 1 pixel thickness
    // and a modified alpha
    v_color.a = min(v_linewidth, v_color.a);
    v_linewidth = max(v_linewidth, 1.0);


    // If color is fully transparent we just will discard the fragment anyway
    if( v_color.a <= 0.0 )
    {
        gl_Position = vec4(0.0,0.0,0.0,1.0);
        return;
    }

    // This is the actual half width of the line
    float w = ceil(1.25*v_antialias+v_linewidth)/2.0;

    vec2 position = a_position;
    vec2 t1 = normalize(a_tangents.xy);
    vec2 t2 = normalize(a_tangents.zw);
    float u = a_texcoord.x;
    float v = a_texcoord.y;
    vec2 o1 = vec2( +t1.y, -t1.x);


    // This is a join
    // ----------------------------------------------------------------
    if( t1 != t2 )
	{
        float angle  = atan (t1.x*t2.y-t1.y*t2.x, t1.x*t2.x+t1.y*t2.y);
        vec2 t  = normalize(t1+t2);
        vec2 o  = vec2( + t.y, - t.x);

        position.xy += v * w * o / cos(angle/2.0);
        if( angle < 0.0 ) {
            if( u == +1.0 ) {
              //  u = v_segment.y + v * w * tan(angle/2.0);
			  u = v_segment.y;
            } else {
              //  u = v_segment.x - v * w * tan(angle/2.0);
			  u = v_segment.x;
            }
        } 
		else
		{
            if( u == +1.0 )
			{
               // u = v_segment.y + u * w * tan(angle/2.0);
				u = v_segment.y;
            }
			 else
			 {
               //u = v_segment.x - u * w * tan(angle/2.0);
			   u = v_segment.x;
             }
        }

    // This is a line start or end (t1 == t2)
    // ------------------------------------------------------------------------
    } 
	else 
	{
        position += v * w * o1;
        if( u == -1.0 ) 
		{
            u = v_segment.x - w;
            position -=  w * t1;
        } 
		else 
		{
            u = v_segment.y + w;
            position +=  w * t2;
        }
    }

    // Miter distance
    // ------------------------------------------------------------------------
    vec2 t;
    vec2 curr = a_position;
    if( a_texcoord.x < 0.0 ) 
	{
        vec2 next = curr + t2*(v_segment.y-v_segment.x);
        v_miter.x = signed_distance(curr, curr+t, position);
        v_miter.y = signed_distance(next, next+t, position);
    } 
	else 
	{
        vec2 prev = curr - t1*(v_segment.y-v_segment.x);
        v_miter.x = signed_distance(prev, prev+t, position);
        v_miter.y = signed_distance(curr, curr+t, position);
    }

    if (!closed && v_segment.x <= 0.0) 
	{
        v_miter.x = 1e10;
    }

    if (!closed && v_segment.y >= v_length)
    {
        v_miter.y = 1e10;
    }

    v_texcoord = vec2( u, v*w );
    gl_Position = u_mvpMatrix*vec4(position,0.0,1.0);
}
