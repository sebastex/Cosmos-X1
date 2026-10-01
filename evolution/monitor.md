# Evolution monitor (generation 1 done, 34.8 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c11 | 8.99 | 4 | 6.8 | 94 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| c3 | 8.77 | 2 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c9 | 8.71 | 2 | 6.5 | 94 | presynaptic_bound 1.0->0.55383, order_gain 4.0->7.71711, order_tau 5.0->10.0, mode_tau 30.0->27.87545, fatigue_gain3 0.0638->0.13156 |
| prime | 8.66 | 4 | 6.5 | 91 | (Prime) |
| c16 | 8.66 | 2 | 6.5 | 94 | plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidation_rate 0.0->0.00569, assembly_inhibition 0.0->0.00063, fire_threshold3 0.0256->0.03678 |
| c10 | 8.56 | 4 | 6.5 | 88 | istdp_rate 50.0->78.43396, consolidated_budget 0.5->0.73875 |
| c5 | 8.54 | 4 | 6.5 | 91 | istdp_target 0.02->0.01304, consolidated_budget 0.5->0.86966, fatigue_gain3 0.0638->0.09735, channel_winners3 4->5 |
| c19 | 8.38 | 2 | 6.0 | 100 | order_gain 4.0->5.43505, mode_tau 30.0->31.77061, channel_winners3 4->5 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 3 | 24 |
| capacity | 2 | 24 |
| efficiency | 2 | 24 |
| wordpairs | 1 | 24 |
| continual | 5 | 24 |
| retention | 11 | 24 |
| order | 1 | 24 |

- A 40% cue brings back on average 72% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): a>k: 3, w>k: 2, z>t: 2, z>m: 1, a>m: 1, m>q: 1, m>e: 1, t>a: 1
- Word pairs recalled wrongly (cue>recalled: times): light>house: 4, green>dream: 4, smile>candy: 4, green>water: 1, plant>tiger: 1, apple>dream: 1, cloud>mouse: 1, plant>mouse: 1

## Picture so far (all generations together)

- Brain tests so far: 48 (20 versions, generation 2).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 4 | 48 | 0 | 4 |
| capacity | 6 | 48 | 0 | 4 |
| efficiency | 2 | 48 | 1 | 4 |
| wordpairs | 1 | 48 | 0 | 4 |
| continual | 12 | 48 | 0 | 4 |
| retention | 18 | 48 | 1 | 4 |
| order | 1 | 48 | 0 | 4 |


### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| plastic_budget | -0.34 | -0.28 | -0.32 |
| fatigue_gain3 | +0.33 | +0.22 | +0.36 |
| inhibition_radius3 | -0.21 | -0.29 | +0.08 |
| winners3 | +0.18 | +0.12 | -0.08 |
| learning_rate | -0.17 | -0.17 | +0.08 |
| order_tau | +0.14 | +0.05 | +0.07 |
| mode_tau | +0.12 | +0.31 | -0.14 |
| soft_bound | +0.11 | -0.12 | +0.17 |
| encoding_suppression | +0.11 | +0.01 | +0.05 |
| fire_threshold3 | +0.11 | +0.11 | -0.17 |
| consolidated_budget | -0.10 | +0.07 | +0.03 |
| presynaptic_bound | -0.10 | -0.02 | -0.09 |
| assembly_inhibition | -0.08 | -0.26 | +0.10 |
| consolidation_rate | -0.07 | +0.05 | -0.24 |
| order_gain | +0.07 | -0.06 | +0.12 |
| hetero_ltd | -0.06 | -0.06 | +0.17 |
| istdp_target | -0.03 | -0.08 | -0.04 |
| channel_winners3 | +0.03 | +0.17 | -0.03 |
| covariance | -0.02 | -0.04 | -0.08 |
| line_recency | +0.00 | +0.05 | +0.04 |
| istdp_rate | -0.00 | -0.07 | +0.11 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.