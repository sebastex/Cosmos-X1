# Evolution monitor (generation 4 done, 45.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c38 | 9.27 | 2 | 7.0 | 94 | soft_bound 1.0->0.85036, presynaptic_bound 1.0->0.96957, istdp_rate 50.0->59.06565, consolidation_rate 0.0->0.00851, order_gain 4.0->3.81883, fire_threshold3 0.0256->0.03696, spread_plastic 0.5->0.59232 |
| c31 | 9.26 | 4 | 7.0 | 94 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |
| c1 | 9.18 | 10 | 7.0 | 90 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c30 | 9.08 | 4 | 7.0 | 88 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->2.85837, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.9167 |
| c36 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c22 | 8.89 | 2 | 7.0 | 75 | learning_rate 0.04->0.03569, plastic_budget 4.0->2.63165, istdp_rate 50.0->59.06565, encoding_suppression 1.0->0.97265, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c28 | 8.82 | 2 | 7.0 | 75 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, line_recency 0.6->0.64844, winners3 3->4 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 3 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 4 | 24 |
| continual | 3 | 24 |
| retention | 4 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 77% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): q>e: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 8, smile>river: 3, smile>mouse: 2, cloud>river: 2, light>house: 2, green>tiger: 2, cloud>candy: 2, green>think: 1

## Picture so far (all generations together)

- Brain tests so far: 120 (44 versions, generation 5).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 16 | 120 | 0 | 10 |
| capacity | 22 | 120 | 0 | 10 |
| efficiency | 5 | 120 | 0 | 10 |
| wordpairs | 17 | 120 | 1 | 10 |
| continual | 27 | 120 | 1 | 10 |
| retention | 29 | 120 | 1 | 10 |
| order | 1 | 120 | 0 | 10 |

- Old memories getting worse (all tests): wiped out -0.013, pushed aside by new memories +0.015 on average; of 27 failed 'keep old memories' tests, 22 were mainly pushed aside (interference), 5 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 787, stored but NOT recalled 171, not stored 2.
- Big-load probe (true word recall %, mean over brains): c1 32 words 70% (n=8); c1 64 words 40% (n=8); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 34% (n=2); c30 64 words 6% (n=2); c31 32 words 75% (n=2); c31 64 words 45% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); prime 32 words 74% (n=10); prime 64 words 32% (n=10)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| inhibition_radius3 | -0.56 | -0.43 | -0.37 |
| order_gain | +0.55 | +0.34 | +0.22 |
| consolidation_rate | -0.47 | -0.30 | -0.21 |
| encoding_suppression | +0.42 | +0.29 | +0.29 |
| separation | -0.33 | -0.26 | -0.11 |
| presynaptic_bound | +0.30 | +0.16 | +0.14 |
| channel_winners3 | -0.24 | -0.09 | -0.09 |
| plastic_budget | +0.22 | +0.06 | +0.14 |
| mode_tau | +0.19 | +0.12 | -0.08 |
| hetero_ltd | +0.19 | +0.20 | +0.05 |
| fatigue_gain3 | +0.16 | -0.02 | +0.17 |
| order_tau | -0.15 | -0.20 | -0.08 |
| fire_threshold3 | -0.15 | -0.10 | -0.06 |
| soft_bound | -0.12 | -0.05 | +0.05 |
| winners3 | +0.11 | +0.19 | -0.02 |
| istdp_rate | +0.10 | +0.20 | +0.06 |
| line_recency | +0.10 | +0.13 | +0.02 |
| consolidated_budget | +0.07 | +0.15 | -0.06 |
| spread_plastic | +0.07 | -0.03 | +0.11 |
| istdp_target | +0.06 | +0.13 | -0.02 |
| learning_rate | -0.05 | -0.03 | -0.06 |
| covariance | +0.03 | +0.03 | +0.13 |
| assembly_inhibition | +0.00 | -0.08 | +0.11 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.