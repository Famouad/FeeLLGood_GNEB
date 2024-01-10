/*
 * Parser/evaluator for the analytic expressions given by the user:
 * the initial magnetization M(x, y, z) and the applied field B(t).
 */

#ifndef expression_parser_h
#define expression_parser_h

#include <duktape.h>
#include <string>

#include <eigen3/Eigen/Dense>

/**
 * This class handles a JavaScript function that computes the three components of a vector, which
 * depends on either a single parameter t or on (x, y, z).
 */
class VectorParser
    {
public:
    VectorParser();
    VectorParser(const VectorParser &) = delete;
    VectorParser &operator=(const VectorParser &) = delete;
    ~VectorParser();

    /**
     * Use the provided JavaScript function expression for subsequent computations of vectors. The
     * provided function should take either one or three numeric parameters, and return an array of
     * three numbers.
     */
    void set_function(const std::string &js_function) const;

    /**
     * Use the provided expressions for the x, y and z components for subsequent computations of
     * vectors. `parameters` should be a comma-separated list of parameter names, typically "t" or
     * "x,y,z".
     */
    void set_expressions(const std::string &parameters, const std::string &expr_x,
                         const std::string &expr_y, const std::string &expr_z);

    /**
     * Compute a vector from the given scalar argument.
     */
    Eigen::Vector3d get_vector(double arg) const;

    /**
     * Compute a vector from the given vector argument.
     */
    Eigen::Vector3d get_vector(const Eigen::Ref<Eigen::Vector3d> arg) const;

private:
    /**
     * Abort with an suitable error message if `err` is an actual error, in which case the top of
     * the stack is assumed to hold the corresponding Error object.
     */
    void die_if_error(duk_int_t err) const;

    /**
     * Get the given component from the array at the top of the stack. Preserve the stack state.
     */
    double get_vector_component(int idx) const;

    /**
     * Compute a vector. This must be called after duk_dup(ctx, -1) and one or more calls to
     * duk_push_number(). `argument_count` should match the number of arguments pushed.
     */
    Eigen::Vector3d compute_vector(int argument_count) const;

    /**
     * Ducktape context holding the internal state of the interpreter.
     */
    duk_context *ctx;
    };

#endif /* expression_parser_h */
