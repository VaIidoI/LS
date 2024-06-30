# Entry point
call: main; exit: 0;

func main():
    # Define the number of terms for the Fibonacci sequence
    var n = 20;
    
    # Initialize the first two terms
    var term1 = 0;
    var term2 = 1;
    var nextTerm = 0;

    # Print the first two terms
    print: "Fibonacci Sequence:"; endl;
    print: term1; endl;
    print: term2; endl;

    # Loop to calculate the next terms of the Fibonacci sequence
    for i = 3, i <= n, i++:
        nextTerm = term1;
        nextTerm += term2;
        
        # Print the next term
        print: nextTerm; endl;

        # Update terms for the next iteration
        term1 = term2;
        term2 = nextTerm;
    end;
end;
