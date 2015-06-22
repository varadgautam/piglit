[require]
GLSL >= ${major}.${minor}

[vertex shader]
varying vec4 color;

% for type_, name, value in type_list:
uniform ${type_} ${name} = ${value};
% endfor

void main()
{
  ## This is a little bit complex too look at, but it's actually pretty simple.
  ## What it does is use the name and type values from type_list, but the values from api_types
  if (${' && '.join('{0} == {1}({2})'.format(n, t, ', '.join(api_types[i][2])) for i, (t, n, _) in enumerate(type_list[1:-1], start=1))}) {
    color = vec4(0, 1, 0, 1);
  } else {
    color = vec4(1, 0, 0, 1);
  }

  gl_Position = gl_Vertex;
}

[fragment shader]
varying vec4 color;

void main()
{
  gl_FragColor = color;
}

[test]
% for api_type, name, values in api_types:
uniform ${api_type} ${name} ${" ".join(values)}
% endfor
draw rect -1 -1 2 2
probe all rgb 0 1 0
