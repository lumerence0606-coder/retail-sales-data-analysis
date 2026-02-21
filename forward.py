# ---------------------------------------------------
# Part a - First-order forward difference
# ---------------------------------------------------
def forward_diff(x, y):
    # Change the line below to your code
    """
    Calculate first derivative using forward difference formula.
    
    Formula: f'(x_i) = (f(x_{i+1}) - f(x_i)) / h (approximation)
    where h = x_{i+1} - x_i
    
    Parameters:
    x : list of floats - data points
    y : list of floats - function values
    
    Returns:
    df : list - approximated derivatives (last element is None)
    """
    df = []
    
   
    for i in range(len(x) - 1):
        
        h = x[i + 1] - x[i]
        derivative = (y[i + 1] - y[i]) / h
        
        
        df.append(derivative)
    
   
    df.append(None)
    
    return df
# ---------------------------------------------------
# Part b - Compute Errors
# ---------------------------------------------------
def forward_diff_secondorder(x, y):
    # Change the line below to your code
    """
    Calculate first derivative using second-order forward difference.
    
    Formula: f'(x_i) = (-3*f(x_i) + 4*f(x_{i+1}) - f(x_{i+2})) / (2*h) (approximation)
    where h = x_{i+1} - x_i
    
    This is more accurate than first-order (O(h^2) vs O(h) error).
    
    Parameters:
    x : list of floats - data points
    y : list of floats - function values
    
    Returns:
    df_second : list - approximated derivatives (last 2 elements are None)
    """
    df_second = []
    
    for i in range(len(x) - 2):
        h = x[i + 1] - x[i]
        derivative = (-3 * y[i] + 4 * y[i + 1] - y[i + 2]) / (2 * h)
        df_second.append(derivative)
    
    df_second.append(None)
    df_second.append(None)
    

    return df_second


####################################################################
## Do not change anything below this part
####################################################################

if __name__ == "__main__":
    x = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
    y = [0.00, 1.15, 2.38, 3.60, 4.82, 6.00]

    # Part a - Test case
    print("-------------------------------")
    print("Forward Part a - Test Output")
    print("-------------------------------")

    df_first = forward_diff(x, y)
    print("Time (s):        ", x)
    print("First-order a(t):", [round(a, 2) if a is not None else None for a in
                                df_first])

    print("\n-------------------------------")
    print("Forward Part b - Test Output")
    print("-------------------------------")
    df_second = forward_diff_secondorder(x, y)
    print("Time (s):        ", x)
    print("Second-order a(t):", [round(a, 2) if a is not None else None for a in
                                 df_second])
