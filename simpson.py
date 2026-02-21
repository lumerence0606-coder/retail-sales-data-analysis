import matplotlib.pyplot as plt

# ---------------------------------------------------
# Part a - Simpson rule
# ---------------------------------------------------
def simpson_integrate(f, a, b, n):
    """
    Apply Simpson's rule to approximate the integral of function f from a to b.
    
    Parameters:
    f: callable function representing f(x)
    a: float, lower limit of integration
    b: float, upper limit of integration
    n: int, number of subintervals (must be even)
    
    Returns:
    approx_int: float, approximate value of the integral
    """
    
    # Calculate step size
    h = (b - a) / n
    
    # Initialize with first and last terms
    approx_int = f(a) + f(b)
    
    # Sum odd-indexed terms (multiply by 4)
    for k in range(1, n, 2):
        x_k = a + k * h
        approx_int += 4 * f(x_k)
    
    # Sum even-indexed terms (multiply by 2)
    for k in range(2, n, 2):
        x_k = a + k * h
        approx_int += 2 * f(x_k)
    
    # Multiply by h/3
    approx_int *= h / 3

    return approx_int

# ---------------------------------------------------
# Part b - Compute Errors
# ---------------------------------------------------
def compute_error(n_values, exact_int, show_fig = False):
    """
    Compute absolute errors for different numbers of subintervals.
    
    Parameters:
    n_values: list of integers specifying subintervals
    exact_int: float, exact value of the integral
    show_fig: bool, whether to display the plot
    
    Returns:
    errors: list of absolute errors
    fig: matplotlib figure object (or None if show_fig is False)
    """
    

    
    # Define the function to integrate: x^3
    f = lambda x: x**3
    a, b = 0, 10
    
    # Compute errors for each n value
    errors = []
    for n in n_values:
        approx = simpson_integrate(f, a, b, n)
        error = abs(exact_int - approx)
        errors.append(error)
    
    # Plot error if show_fig = True
    if show_fig:
        # Initialize figure
        fig = plt.figure(figsize=(10, 6))
        
        # Create plot
        plt.plot(n_values, errors, 'bo-', linewidth=2, markersize=8, label='Absolute Error')
        plt.xlabel('Number of Subintervals (n)', fontsize=12)
        plt.ylabel('Absolute Error', fontsize=12)
        plt.title('Simpson\'s Rule: Absolute Error vs Number of Subintervals', fontsize=14)
        plt.grid(True, alpha=0.3)
        plt.legend(fontsize=10)
        plt.yscale('log')  # Use log scale for better visualization of small errors
        plt.tight_layout()
        plt.show(block=False)  # Show plot without blocking execution
        plt.pause(0.1)  # Small pause to ensure plot displays
    else:
        fig = None

    return errors, fig

# Create plot with suitable n_values for analysis
n_values_for_plot = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000]
print("\n\n--------------------------")
print("Generating plot for Part c")
print("------------------------------")
errors_for_plot, fig_for_assignment = compute_error(n_values_for_plot, 10**4 / 4, show_fig=True)
print(f"Errors for plot: {errors_for_plot}")

####################################################################
## Do not change anything below this part
####################################################################

import numpy as np

if __name__ == "__main__":

    # Part a - Test case
    print("\n\n--------------------------")
    print("Simpson Part a - Test Output")
    print("\n\n--------------------------")
    f = lambda x: np.exp(-(x**2))
    a, b, n = 0, 1, 50

    integral = simpson_integrate(f, a, b, n)

    print("Integration of f(x) = e^{-x^2} from 0 to 1")
    print("yields approximately", integral)

    # Part b - Test case
    print("\n\n--------------------------")
    print("Simpson Part b - Test Output")
    print("------------------------------")
    n_values = [2, 4, 6, 8, 10]

    err, fig_output = compute_error(n_values, 10**4 / 4, False)

    print(f"Errors: {err}")
