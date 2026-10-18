module op_div;

extern (C) int op_div_d(void* ctx, const(double)* args, size_t argc, double* out_param) {
    if (args[1] == 0.0) {
        return -1;
    }
    *out_param = args[0] / args[1];
    return 0;
}

