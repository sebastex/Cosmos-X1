# Evolution monitor (generation 6 done, 43.0 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c57 | 9.28 | 2 | 7.0 | 94 | learning_rate 0.04->0.03434, soft_bound 1.0->0.887, consolidated_budget 0.5->0.54388, assembly_inhibition 0.0->0.00027, order_gain 4.0->4.56564, separation 3.0->0.5762 |
| c44 | 9.12 | 2 | 7.0 | 94 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->3.77761, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.73329, winners3 3->4 |
| c52 | 9.09 | 2 | 7.0 | 94 | learning_rate 0.04->0.02824, soft_bound 1.0->0.887, istdp_rate 50.0->109.70149, order_gain 4.0->4.7958, mode_tau 30.0->24.00286, winners3 3->4, separation 3.0->0.5762 |
| c46 | 9.09 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->3.19911, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->4.49026, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.54988, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.5211 |
| c30 | 9.08 | 4 | 7.0 | 88 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->2.85837, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.9167 |
| c1 | 9.05 | 14 | 6.9 | 91 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c36 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c48 | 9.02 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.01572, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.06565, assembly_inhibition 0.0->0.00132, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->44.6549, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 7 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 5 | 24 |
| continual | 4 | 24 |
| retention | 4 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 67% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): t>k: 2, w>a: 2, a>e: 1, k>e: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 10, cloud>candy: 4, smile>river: 3, bench>river: 2, plant>river: 2, smile>mouse: 2, cloud>mouse: 2, plant>think: 2

## Picture so far (all generations together)

- Brain tests so far: 168 (60 versions, generation 7).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 19 | 168 | 0 | 14 |
| capacity | 30 | 168 | 1 | 14 |
| efficiency | 7 | 168 | 0 | 14 |
| wordpairs | 22 | 168 | 1 | 14 |
| continual | 33 | 168 | 2 | 14 |
| retention | 36 | 168 | 1 | 14 |
| order | 1 | 168 | 0 | 14 |

- Old memories getting worse (all tests): wiped out -0.012, pushed aside by new memories +0.015 on average; of 33 failed 'keep old memories' tests, 26 were mainly pushed aside (interference), 7 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 1115, stored but NOT recalled 227, not stored 2.
- Big-load probe (true word recall %, mean over brains): c1 32 words 69% (n=12); c1 64 words 40% (n=12); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 34% (n=2); c30 64 words 6% (n=2); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); prime 32 words 75% (n=14); prime 64 words 33% (n=14)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| inhibition_radius3 | -0.49 | -0.37 | -0.30 |
| order_gain | +0.45 | +0.30 | +0.19 |
| consolidation_rate | -0.42 | -0.28 | -0.18 |
| encoding_suppression | +0.39 | +0.28 | +0.25 |
| separation | -0.31 | -0.29 | -0.10 |
| presynaptic_bound | +0.28 | +0.14 | +0.12 |
| plastic_budget | +0.23 | +0.10 | +0.19 |
| hetero_ltd | +0.18 | +0.20 | +0.04 |
| mode_tau | +0.15 | +0.05 | -0.02 |
| channel_winners3 | -0.15 | -0.08 | -0.04 |
| learning_rate | -0.13 | -0.12 | -0.08 |
| soft_bound | -0.13 | -0.04 | +0.03 |
| fatigue_gain3 | +0.12 | -0.02 | +0.15 |
| winners3 | +0.11 | +0.16 | +0.01 |
| order_tau | -0.09 | -0.19 | -0.08 |
| istdp_rate | +0.08 | +0.17 | +0.07 |
| spread_plastic | +0.05 | -0.10 | -0.01 |
| covariance | +0.05 | +0.06 | +0.11 |
| line_recency | +0.05 | +0.15 | +0.03 |
| fire_threshold3 | -0.05 | -0.05 | -0.06 |
| consolidated_budget | +0.01 | +0.12 | -0.03 |
| istdp_target | -0.01 | +0.07 | -0.06 |
| assembly_inhibition | -0.00 | -0.08 | +0.11 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.