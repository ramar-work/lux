CREATE TABLE IF NOT EXISTS general_election
(
	votes integer,
	precinct_count integer,
	party text,
	dist_type text,
	dist_code text,
	contest_name text,
	cand_num integer,
	cand_name text,
	vote_for integer,
	ref text
);
