# Evolution monitor (generation 2 done, 45.5 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| prime | 9.73 | 6 | 1.2 | 100 | (Prime) |
| c5 | 9.67 | 6 | 1.0 | 98 | hetero_ltd 0.9733->0.99241, consolidated_budget 0.50842->0.5, mode_tau 13.44333->19.44795, winners3 5->6, spread_plastic 0.45398->0.25208 |
| c1 | 9.65 | 6 | 1.2 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->1.0, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.07187, silence_gate 0.6->0.29702 |
| c2 | 9.65 | 4 | 1.0 | 100 | consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, silence_gate 0.6->0.7538 |
| c15 | 9.54 | 2 | 1.0 | 100 | learning_rate 0.02222->0.02412, presynaptic_bound 0.89812->0.8741, covariance 0.58435->0.79134, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, fire_threshold3 0.02107->0.03121, channel_winners3 2->3, spread_plastic 0.45398->0.56147, link_bound 0.3->0.14662 |
| c7 | 9.47 | 2 | 1.0 | 94 | order_gain 6.23347->5.58834, order_tau 2.90192->2.16953, mode_tau 13.44333->10.0, channel_winners3 2->3, link_bound 0.3->0.61169 |
| c9 | 9.46 | 2 | 1.0 | 100 | consolidation_rate 0.0056->0.0, winners3 5->4, spread_plastic 0.45398->0.49669, link_bound 0.3->0.49157 |
| c4 | 9.46 | 4 | 1.0 | 97 | learning_rate 0.02222->0.02412, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, channel_winners3 2->3, link_bound 0.3->0.19534 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 16 | 16 |
| wordpairs | 0 | 16 |

- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 3

## Picture so far (all generations together)

- Brain tests so far: 48 (16 versions, generation 3).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 46 | 48 | 5 | 6 |
| wordpairs | 1 | 48 | 0 | 6 |

- Word pairs (8 pairs per test, all tests): stored and recalled 374, stored but NOT recalled 10, not stored 0.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.62 | +nan | +nan |
| inhibition_radius3 | -0.57 | +nan | +nan |
| presynaptic_bound | -0.30 | +nan | +nan |
| line_recency | -0.28 | +nan | +nan |
| covariance | -0.20 | +nan | +nan |
| mode_tau | +0.20 | +nan | +nan |
| winners3 | +0.19 | +nan | +nan |
| spread_plastic | -0.15 | +nan | +nan |
| consolidation_rate | -0.13 | +nan | +nan |
| learning_rate | +0.13 | +nan | +nan |
| fire_threshold3 | +0.11 | +nan | +nan |
| fatigue_gain3 | +0.11 | +nan | +nan |
| consolidated_budget | -0.10 | +nan | +nan |
| silence_gate | -0.10 | +nan | +nan |
| hetero_ltd | +0.08 | +nan | +nan |
| separation | -0.08 | +nan | +nan |
| order_gain | -0.05 | +nan | +nan |
| order_tau | -0.05 | +nan | +nan |
| plastic_budget | -0.04 | +nan | +nan |
| assembly_inhibition | -0.03 | +nan | +nan |
| link_bound | +0.02 | +nan | +nan |
| channel_winners3 | +0.01 | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.