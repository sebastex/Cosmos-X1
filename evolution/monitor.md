# Evolution monitor (generation 0 done, 47.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| prime | 10.42 | 2 | 1.5 | 100 | (Prime) |
| c1 | 10.42 | 2 | 1.5 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->1.0, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.07187, silence_gate 0.6->0.29702 |
| c2 | 9.85 | 2 | 1.0 | 100 | consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, silence_gate 0.6->0.7538 |
| c5 | 9.84 | 2 | 1.0 | 100 | hetero_ltd 0.9733->0.99241, consolidated_budget 0.50842->0.5, mode_tau 13.44333->19.44795, winners3 5->6, spread_plastic 0.45398->0.25208 |
| c4 | 9.69 | 2 | 1.0 | 100 | learning_rate 0.02222->0.02412, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, channel_winners3 2->3, link_bound 0.3->0.19534 |
| c7 | 9.47 | 2 | 1.0 | 94 | order_gain 6.23347->5.58834, order_tau 2.90192->2.16953, mode_tau 13.44333->10.0, channel_winners3 2->3, link_bound 0.3->0.61169 |
| c3 | 8.84 | 2 | 1.0 | 100 | learning_rate 0.02222->0.0161, consolidation_rate 0.0056->0.06832, line_recency 0.48403->0.52527, inhibition_radius3 2->3 |
| c6 | 8.63 | 2 | 0.5 | 75 | plastic_budget 5.67831->6.2017, presynaptic_bound 0.89812->1.0, covariance 0.58435->1.0, line_recency 0.48403->0.95343, link_bound 0.3->0.33422 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 14 | 16 |
| wordpairs | 1 | 16 |

- Word pairs recalled wrongly (cue>recalled: times): storm>mouse: 1, plant>think: 1, smile>mouse: 1, cloud>candy: 1, smile>candy: 1

## Picture so far (all generations together)

- Brain tests so far: 16 (8 versions, generation 1).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 14 | 16 | 1 | 2 |
| wordpairs | 1 | 16 | 0 | 2 |

- Word pairs (8 pairs per test, all tests): stored and recalled 123, stored but NOT recalled 5, not stored 0.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| presynaptic_bound | -0.59 | +nan | +nan |
| line_recency | -0.55 | +nan | +nan |
| plastic_budget | -0.51 | +nan | +nan |
| covariance | -0.51 | +nan | +nan |
| consolidation_rate | -0.41 | +nan | +nan |
| inhibition_radius3 | -0.40 | +nan | +nan |
| hetero_ltd | +0.40 | +nan | +nan |
| fatigue_gain3 | +0.39 | +nan | +nan |
| learning_rate | +0.38 | +nan | +nan |
| silence_gate | -0.29 | +nan | +nan |
| mode_tau | +0.15 | +nan | +nan |
| link_bound | -0.14 | +nan | +nan |
| consolidated_budget | -0.11 | +nan | +nan |
| spread_plastic | -0.10 | +nan | +nan |
| winners3 | +0.10 | +nan | +nan |
| order_tau | +0.09 | +nan | +nan |
| channel_winners3 | -0.05 | +nan | +nan |
| order_gain | +0.05 | +nan | +nan |
| assembly_inhibition | +0.02 | +nan | +nan |
| encoding_suppression | +nan | +nan | +nan |
| fire_threshold3 | +nan | +nan | +nan |
| separation | +nan | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.