# Evolution monitor (generation 0 done, 11.4 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c5 | 9.23 | 2 | 7.0 | 94 | istdp_target 0.02->0.01304, consolidated_budget 0.5->0.86966, fatigue_gain3 0.0638->0.09735, channel_winners3 4->5 |
| c10 | 9.15 | 2 | 7.0 | 88 | istdp_rate 50.0->78.43396, consolidated_budget 0.5->0.73875 |
| c11 | 9.13 | 2 | 7.0 | 88 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| c3 | 8.77 | 2 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c9 | 8.71 | 2 | 6.5 | 94 | presynaptic_bound 1.0->0.55383, order_gain 4.0->7.71711, order_tau 5.0->10.0, mode_tau 30.0->27.87545, fatigue_gain3 0.0638->0.13156 |
| prime | 8.61 | 2 | 6.5 | 88 | (Prime) |
| c7 | 8.12 | 2 | 6.0 | 88 | hetero_ltd 1.0->0.78061, istdp_rate 50.0->65.58294, fire_threshold3 0.0256->0.04215 |
| c1 | 7.93 | 2 | 6.0 | 82 | order_gain 4.0->8.0, mode_tau 30.0->30.92911 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 1 | 24 |
| capacity | 4 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 0 | 24 |
| continual | 7 | 24 |
| retention | 7 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 72% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): t>w: 1, e>q: 1, k>z: 1, z>w: 1, e>a: 1, k>m: 1, m>t: 1, e>t: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 20, green>mouse: 2, cloud>mouse: 1, cloud>candy: 1, smile>river: 1
