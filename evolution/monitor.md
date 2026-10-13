# Evolution monitor (generation 9 done, 39.4 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c40 | 10.42 | 2 | 1.5 | 100 | learning_rate 0.02222->0.02519, plastic_budget 5.67831->3.0247, presynaptic_bound 0.89812->0.8741, hetero_ltd 0.9733->0.75833, covariance 0.58435->0.64581, consolidation_rate 0.0056->0.04061, consolidated_budget 0.50842->0.5, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, fire_threshold3 0.02107->0.02206, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.56147, silence_gate 0.6->0.48732 |
| c38 | 10.38 | 4 | 1.5 | 100 | learning_rate 0.02222->0.02412, plastic_budget 5.67831->7.92854, presynaptic_bound 0.89812->0.8741, hetero_ltd 0.9733->0.94764, covariance 0.58435->0.64581, consolidated_budget 0.50842->0.50947, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, mode_tau 13.44333->11.86193, fire_threshold3 0.02107->0.03121, line_recency 0.48403->0.4, spread_plastic 0.45398->0.56147, link_bound 0.3->0.12925, silence_gate 0.6->0.48732 |
| c39 | 10.11 | 4 | 1.5 | 100 | learning_rate 0.02222->0.01569, plastic_budget 5.67831->6.07006, hetero_ltd 0.9733->0.99241, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->4.08064, mode_tau 13.44333->16.33853, fire_threshold3 0.02107->0.02221, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->2.9207, spread_plastic 0.45398->0.25208, silence_gate 0.6->0.83019 |
| c17 | 10.02 | 14 | 1.2 | 100 | learning_rate 0.02222->0.02412, plastic_budget 5.67831->7.92854, presynaptic_bound 0.89812->0.8741, covariance 0.58435->0.79134, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, fire_threshold3 0.02107->0.03121, spread_plastic 0.45398->0.56147, link_bound 0.3->0.12925, silence_gate 0.6->0.48732 |
| c36 | 9.77 | 2 | 1.0 | 100 | learning_rate 0.02222->0.01569, plastic_budget 5.67831->3.0247, presynaptic_bound 0.89812->0.80421, hetero_ltd 0.9733->0.75833, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208, silence_gate 0.6->0.3818 |
| c28 | 9.73 | 4 | 1.0 | 100 | learning_rate 0.02222->0.01569, hetero_ltd 0.9733->0.99241, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->4.08064, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208 |
| c1 | 9.71 | 14 | 1.1 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->1.0, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.07187, silence_gate 0.6->0.29702 |
| c5 | 9.70 | 14 | 1.1 | 97 | hetero_ltd 0.9733->0.99241, consolidated_budget 0.50842->0.5, mode_tau 13.44333->19.44795, winners3 5->6, spread_plastic 0.45398->0.25208 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 13 | 16 |
| wordpairs | 0 | 16 |


## Picture so far (all generations together)

- Brain tests so far: 160 (44 versions, generation 10).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 146 | 160 | 18 | 20 |
| wordpairs | 3 | 160 | 0 | 20 |

- Word pairs (8 pairs per test, all tests): stored and recalled 1255, stored but NOT recalled 25, not stored 0.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.79 | +nan | +nan |
| link_bound | -0.39 | +nan | +nan |
| inhibition_radius3 | -0.27 | +nan | +nan |
| separation | -0.26 | +nan | +nan |
| order_gain | +0.25 | +nan | +nan |
| covariance | +0.24 | +nan | +nan |
| channel_winners3 | -0.23 | +nan | +nan |
| fire_threshold3 | +0.18 | +nan | +nan |
| assembly_inhibition | +0.17 | +nan | +nan |
| order_tau | +0.14 | +nan | +nan |
| presynaptic_bound | -0.14 | +nan | +nan |
| fatigue_gain3 | +0.13 | +nan | +nan |
| silence_gate | -0.12 | +nan | +nan |
| plastic_budget | +0.11 | +nan | +nan |
| spread_plastic | +0.10 | +nan | +nan |
| line_recency | -0.09 | +nan | +nan |
| consolidation_rate | +0.09 | +nan | +nan |
| mode_tau | +0.07 | +nan | +nan |
| hetero_ltd | -0.06 | +nan | +nan |
| winners3 | +0.03 | +nan | +nan |
| consolidated_budget | -0.01 | +nan | +nan |
| learning_rate | -0.01 | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.