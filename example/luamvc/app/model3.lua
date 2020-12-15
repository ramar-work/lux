return {
	clients = {
		{
			client = "Lisa Sampson",

			loans  = {
				{ type = "mortgage", amount_owed = 28000, interest = 0.05 },
				{ type = "credit",   amount_owed = 1567, interest = 0.18 },
			},

			membership = {
				type = "partial",
				length = 11,
				home = "Richmond, VA"
			}
		},

		{
			client = "Joseph Waterton",

			loans  = {
				{ type = "mortgage", amount_owed = 11000, interest = 0.03 },
				{ type = "credit",   amount_owed = 320, interest = 0.23 },
				{ type = "credit",   amount_owed = 567, interest = 0.08 },
			},

			membership = {
				type = "full",
				length = 3,
				home = "Arlington, VA"
			}
		}
	}
}
