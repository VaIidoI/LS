var max = 100000; var primeCount = 0; var root = 0; var isPrime = false; var modulo = 0;
call: main; exit: 0;

func main() { 
	for (n = 2, n < max, n++) {
		root = n; sqrt: root; isPrime = true;
		for (i = 2, i <= root, i++) {
			modulo = n; modulo %= i;
			
			if (modulo == 0) {
				isPrime = false; break;
			}
		}
		
		if (isPrime) {
			primeCount++;
		}
	}
	print: "In the first "; print: max; print: " numbers, there are "; print: primeCount; print: " prime numbers";
}

