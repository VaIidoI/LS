for i = 0, i < 100, i++:
	for j = 0, j < 100, j++:
		var result = j; result *= i;
		#printl: result;
		delete: result;
	end;
end;