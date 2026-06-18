




import matplotlib.pyplot as plt



def bday(n, ss):
	if not hasattr(bday, "last_n"):
		bday.last_n = 0
	if not hasattr(bday, "last_result"):
		bday.last_result = 1

	print(f'calculating bday for', n, 'positions')

	accum = bday.last_result
	divs = n - bday.last_n
	for i in range(bday.last_n, n):
		accum *= (ss-i)
		if(accum > pow(10,40)):
			accum /= pow(ss,2)
			divs -= 2
	accum /= pow(ss,divs)

	bday.last_n = n
	bday.last_result = accum

	prob = 1-accum
	print(f'\tp =', prob)
	return prob





def make_bday_curve(max_inputs, search_space, nsteps):

	incr = int(max_inputs / nsteps)

	x = list(range(incr, max_inputs+1, incr))
	y = [bday(num, search_space) for num in x]


	fig, ax = plt.subplots(figsize=(8, 5))
	ax.plot(x, y, color='blue', linestyle='-', linewidth=2, label='Sine Wave')
	ax.set_title('Probability of hash collisions', fontsize=14, fontweight='bold')
	ax.set_xlabel('num positions', fontsize=11)
	ax.set_ylabel('Pcoll (64-bit hash)', fontsize=11)

	# 5. Add supporting visual elements
	ax.grid(True, linestyle=':', alpha=0.6)  # Light grid lines
	#ax.legend(loc='upper right')             # Display the series labels

	# 6. Save and display the visualization
	#plt.savefig('trig_plot.png', dpi=300)    # Saves the plot with high resolution
	plt.show()


#make_bday_curve(100, 365, 365)

ss = pow(2,64)
nsteps = 8
max_positions = 4000*pow(10, 6)
make_bday_curve(max_positions, ss, nsteps)
