<h2>List</h2>
<!-- Loops through all values regardless of whats there -->
<table>
	<!-- Not sure how to handle models yet -->
	<thead>
	{{$ value:first }}
		<td>{{$ key }}</td>
	{{/ value }}
	</thead>

	<!-- See what's going on.  This defeats the simple purpose, but does the trick -->
	<tbody>
	{{$ value }}
		<tr>
			<td>{{$ key }}</td>
			<td>{{$ value }}</td>
		</tr>
	{{/ value }}
	</tbody>
</table>
