<!-- Lua should run this, not sure how yet... -->
<h2>{{! str.upper .title }}</h2>

<table>
	<thead>
		<th>Classification</td>
	{{# columns }}
		<th>{{$ value }}</th>
	{{/ columns }}
	</thead>

	<tbody>
		<tr>
			<td></td>
		</tr>
	{{# data[ :: ] }}	
		<tr>
			<td>{{$ value }}</td> 
		</tr>
	{{/ data }}	
	</tbody>	
</table>
