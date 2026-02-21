import matplotlib.pyplot as plt

# ---------------------------------------------------
# Part a - Fibonacci
# ---------------------------------------------------
def fibonacci(n):
    # Change the line below to your code
    
    # Base cases
    if n == 0:
        fn = 0
        return fn
    
    if n == 1:
        fn = 1
        return fn
    
    # Use iteration for n >= 2
    prev_prev = 0
    prev = 1
    
    for i in range(2, n + 1):
        fn = prev + prev_prev
        prev_prev = prev
        prev = fn

    return fn

# ---------------------------------------------------
# Part b - Fibonacci ratio and plot
# ---------------------------------------------------
def fibonacci_ratio(n_values, show_fig = False):
    # Change the line below to your code
    ratios = []
    
    # Calculate ratios for each n
    for n in n_values:
        f_n = fibonacci(n)
        f_n_plus_1 = fibonacci(n + 1)
        ratio = f_n_plus_1 / f_n
        ratios.append(ratio)
    
    fig = None
    
    # Plot if requested
    if show_fig:
        fig = plt.figure()
        
        n_list = list(n_values)
        plt.plot(n_list, ratios, 'ro-')
        
        golden_ratio = (1 + 5**0.5) / 2
        plt.axhline(y=golden_ratio, color='gold', linestyle='--', label='Golden Ratio')
        
        plt.xlabel('n')
        plt.ylabel('Ratio F(n+1)/F(n)')
        plt.title('Fibonacci Ratios Converging to Golden Ratio')
        plt.legend()
        plt.grid(True)
        plt.show()

    return ratios, fig


####################################################################
## Do not change anything below this part
####################################################################

if __name__ == "__main__":

    # Part a - Test case
    print("-------------------------------")
    print("Fibonacci Part a - Test Output")
    print("-------------------------------")

    f50 = fibonacci(50)
    f10 = fibonacci(10)

    print("The value of F10 is", f10)
    print("The value of F50 is", f50)

    # Part b - Test case
    print("\n\n---------------------------")
    print("Fibonacci Part b - Test Output")
    print("-------------------------------")

    n_values = range(1, 5)

    ratios, fig = fibonacci_ratio(n_values, True)

    print("The ratios are", [round(x, 3) for x in ratios])