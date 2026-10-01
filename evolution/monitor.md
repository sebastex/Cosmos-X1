# Evolution monitor (generation 2 done, 34.1 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c26 | 9.12 | 2 | 7.0 | 94 | learning_rate 0.04->0.06267, plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidated_budget 0.5->0.6357, assembly_inhibition 0.0->0.00063, order_gain 4.0->4.32059, mode_tau 30.0->25.67333, fire_threshold3 0.0256->0.03678 |
| c11 | 9.10 | 6 | 6.8 | 96 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| prime | 8.85 | 6 | 6.7 | 94 | (Prime) |
| c21 | 8.76 | 2 | 6.5 | 94 | learning_rate 0.04->0.0391, plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.73385, assembly_inhibition 0.0->0.00063, order_tau 5.0->4.69642, mode_tau 30.0->25.62044, fatigue_gain3 0.0638->0.07178, line_recency 0.6->0.56917, channel_winners3 4->3 |
| c3 | 8.68 | 4 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c27 | 8.67 | 2 | 6.5 | 88 | learning_rate 0.04->0.0391, covariance 1.0->0.89106, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917, channel_winners3 4->3 |
| c16 | 8.66 | 2 | 6.5 | 94 | plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidation_rate 0.0->0.00569, assembly_inhibition 0.0->0.00063, fire_threshold3 0.0256->0.03678 |
| c23 | 8.63 | 2 | 6.5 | 88 | plastic_budget 4.0->3.9007, order_tau 5.0->7.40628, fire_threshold3 0.0256->0.0298 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 4 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 0 | 24 |
| continual | 5 | 24 |
| retention | 8 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 77% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): e>w: 4, t>a: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 5, light>house: 5, cloud>candy: 3, smile>mouse: 1, cloud>think: 1, apple>candy: 1

## Picture so far (all generations together)

- Brain tests so far: 72 (28 versions, generation 3).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 6 | 72 | 0 | 6 |
| capacity | 10 | 72 | 0 | 6 |
| efficiency | 3 | 72 | 1 | 6 |
| wordpairs | 1 | 72 | 0 | 6 |
| continual | 17 | 72 | 0 | 6 |
| retention | 26 | 72 | 1 | 6 |
| order | 1 | 72 | 0 | 6 |

- Old memories getting worse (all tests): wiped out -0.006, pushed aside by new memories +0.034 on average; of 17 failed 'keep old memories' tests, 5 were mainly pushed aside (interference), 12 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 176, stored but NOT recalled 16, not stored 0.
- Big-load probe (true word recall %, mean over brains): c11 32 words 88% (n=2); c11 64 words 41% (n=2); c3 32 words 81% (n=2); c3 64 words 36% (n=2); c9 32 words 69% (n=2); c9 64 words 22% (n=2); prime 32 words 78% (n=2); prime 64 words 36% (n=2)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| plastic_budget | -0.35 | -0.32 | -0.26 |
| inhibition_radius3 | -0.31 | -0.24 | -0.08 |
| consolidation_rate | -0.26 | -0.22 | -0.26 |
| soft_bound | +0.24 | +0.10 | +0.20 |
| fatigue_gain3 | +0.20 | +0.08 | +0.26 |
| consolidated_budget | -0.15 | +0.03 | +0.00 |
| winners3 | +0.15 | +0.10 | -0.07 |
| istdp_rate | +0.13 | +0.17 | +0.09 |
| order_tau | +0.09 | -0.04 | +0.13 |
| istdp_target | +0.07 | -0.00 | +0.02 |
| assembly_inhibition | -0.07 | -0.21 | +0.07 |
| encoding_suppression | +0.06 | -0.00 | +0.19 |
| channel_winners3 | -0.06 | +0.07 | -0.06 |
| order_gain | +0.05 | -0.08 | +0.13 |
| presynaptic_bound | -0.05 | +0.07 | -0.11 |
| hetero_ltd | +0.05 | +0.05 | +0.17 |
| line_recency | -0.04 | -0.01 | +0.03 |
| covariance | -0.04 | -0.03 | -0.09 |
| mode_tau | -0.01 | +0.14 | -0.12 |
| fire_threshold3 | -0.01 | -0.03 | -0.13 |
| learning_rate | +0.01 | -0.02 | +0.14 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.