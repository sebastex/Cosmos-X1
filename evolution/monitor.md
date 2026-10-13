# Evolution monitor (generation 1 done, 46.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c1 | 9.94 | 4 | 1.2 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->1.0, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.07187, silence_gate 0.6->0.29702 |
| prime | 9.94 | 4 | 1.2 | 100 | (Prime) |
| c5 | 9.73 | 4 | 1.0 | 100 | hetero_ltd 0.9733->0.99241, consolidated_budget 0.50842->0.5, mode_tau 13.44333->19.44795, winners3 5->6, spread_plastic 0.45398->0.25208 |
| c4 | 9.69 | 2 | 1.0 | 100 | learning_rate 0.02222->0.02412, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, channel_winners3 2->3, link_bound 0.3->0.19534 |
| c2 | 9.65 | 4 | 1.0 | 100 | consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, silence_gate 0.6->0.7538 |
| c7 | 9.47 | 2 | 1.0 | 94 | order_gain 6.23347->5.58834, order_tau 2.90192->2.16953, mode_tau 13.44333->10.0, channel_winners3 2->3, link_bound 0.3->0.61169 |
| c9 | 9.46 | 2 | 1.0 | 100 | consolidation_rate 0.0056->0.0, winners3 5->4, spread_plastic 0.45398->0.49669, link_bound 0.3->0.49157 |
| c10 | 9.45 | 2 | 1.0 | 100 | plastic_budget 5.67831->4.08371, hetero_ltd 0.9733->0.85752, consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.5, mode_tau 13.44333->17.34619, winners3 5->6, channel_winners3 2->3 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 16 | 16 |
| wordpairs | 0 | 16 |

- Word pairs recalled wrongly (cue>recalled: times): light>house: 1, smile>river: 1

## Picture so far (all generations together)

- Brain tests so far: 32 (12 versions, generation 2).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 30 | 32 | 3 | 4 |
| wordpairs | 1 | 32 | 0 | 4 |

- Word pairs (8 pairs per test, all tests): stored and recalled 249, stored but NOT recalled 7, not stored 0.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.67 | +nan | +nan |
| inhibition_radius3 | -0.64 | +nan | +nan |
| presynaptic_bound | -0.60 | +nan | +nan |
| line_recency | -0.30 | +nan | +nan |
| covariance | -0.23 | +nan | +nan |
| learning_rate | +0.22 | +nan | +nan |
| fatigue_gain3 | +0.18 | +nan | +nan |
| silence_gate | -0.17 | +nan | +nan |
| consolidation_rate | -0.15 | +nan | +nan |
| mode_tau | +0.14 | +nan | +nan |
| hetero_ltd | +0.10 | +nan | +nan |
| winners3 | +0.10 | +nan | +nan |
| consolidated_budget | -0.10 | +nan | +nan |
| plastic_budget | -0.10 | +nan | +nan |
| spread_plastic | -0.10 | +nan | +nan |
| assembly_inhibition | +0.09 | +nan | +nan |
| order_gain | +0.08 | +nan | +nan |
| channel_winners3 | +0.07 | +nan | +nan |
| link_bound | -0.05 | +nan | +nan |
| order_tau | -0.01 | +nan | +nan |
| fire_threshold3 | +nan | +nan | +nan |
| separation | +nan | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.