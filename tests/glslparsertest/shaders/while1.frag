// [config]
// expect_result: fail
// glsl_version: 1.10
//
// [end config]

void main()
{
    int i;
    while(i) {  // condition should be boolean
    }
}
