for i = 0, i < 100, i++: 
	for j = i, j < 100, j++:
		var total = j; total *= i;
		printl: total;
		delete: total;
	end;
end;


