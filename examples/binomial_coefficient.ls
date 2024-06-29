# Entry point
call: main; return;

func factorial(number):
    var result = 1;

    for i = 1, i <= number, i++:
        result *= i;
    end;

    push: result; delete: result;
end;

func main():
    # Define two numbers n and k for binomial coefficient calculation
    var n = 5;
    var k = 3;
	
	var factN; var factK; var factNK;
	
    # Call the factorial function for n, k, and (n-k)
    factorial(n); pop: factN;
    factorial(k); pop: factK;
    var nk = n; nk -= k;
    factorial(nk); pop: factNK;

    # Calculate the binomial coefficient
    var denominator = factK; denominator *= factNK;
    var binomCoeff = factN; binomCoeff /= denominator;

    # Print the result
    print: "Binomial Coefficient ("; print: n; print: " choose "; print: k; print: ") is "; print: binomCoeff; endl;
end;
