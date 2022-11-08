<h1>{{ .title }}</h1>
{{# clients }}
<h2>{{ .client }}</h2>

<table>
	<thead>
		<th class="fish">Type</th>
		<th class="fish">Amount_owed</th>
		<th class="fish">Interest</th>
	</thead>
	<tbody>
	{{# .loans }}
		<tr>
			<td class="fish">{{ .type }}</td>
			<td class="fish">{{ .amount_owed }}</td>
			<td class="fish">{{ .interest }}</td>
		</tr>
	{{/ .loans }}
	</tbody>
</table>
{{/ clients }}
