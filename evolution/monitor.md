# Evolution monitor (generation 9 done, 39.5 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c79 | 9.07 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.67792, winners3 3->4, channel_winners3 4->3, separation 3.0->4.07163, spread_plastic 0.5->0.1191 |
| c76 | 9.01 | 2 | 7.0 | 88 | learning_rate 0.04->0.06186, istdp_rate 50.0->25.31027, order_gain 4.0->3.9856, order_tau 5.0->5.23535, mode_tau 30.0->29.14217, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c83 | 9.01 | 2 | 7.0 | 82 | learning_rate 0.04->0.07419, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02736, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.04192, assembly_inhibition 0.0->9e-05, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.73866, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.10301 |
| c1 | 9.00 | 16 | 6.8 | 91 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c51 | 8.98 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.10301 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c31 | 8.98 | 8 | 6.9 | 88 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |
| c78 | 8.97 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->73.13865, istdp_target 0.02->0.02435, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.81946, mode_tau 30.0->39.62321, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 3 | 24 |
| efficiency | 3 | 24 |
| wordpairs | 2 | 24 |
| continual | 4 | 24 |
| retention | 4 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 76% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): k>z: 2, m>t: 2, z>w: 1, z>q: 1, z>m: 1, w>a: 1, a>z: 1, e>a: 1
- Word pairs recalled wrongly (cue>recalled: times): cloud>water: 13, smile>candy: 11, cloud>candy: 2, smile>river: 2, cloud>river: 2, green>mouse: 1, light>house: 1

## Picture so far (all generations together)

- Brain tests so far: 240 (84 versions, generation 10).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 29 | 240 | 0 | 20 |
| capacity | 41 | 240 | 1 | 20 |
| efficiency | 11 | 240 | 1 | 20 |
| wordpairs | 37 | 240 | 1 | 20 |
| continual | 51 | 240 | 2 | 20 |
| retention | 55 | 240 | 2 | 20 |
| order | 1 | 240 | 0 | 20 |

- Old memories getting worse (all tests): wiped out -0.017, pushed aside by new memories +0.015 on average; of 51 failed 'keep old memories' tests, 38 were mainly pushed aside (interference), 13 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 1574, stored but NOT recalled 341, not stored 5.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=14); c1 64 words 40% (n=14); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c36 32 words 59% (n=2); c36 64 words 30% (n=2); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 58% (n=4); c46 64 words 40% (n=4); c48 32 words 56% (n=2); c48 64 words 34% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); prime 32 words 70% (n=20); prime 64 words 30% (n=20)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.48 | +0.36 | +0.33 |
| inhibition_radius3 | -0.41 | -0.30 | -0.25 |
| order_gain | +0.35 | +0.24 | +0.16 |
| consolidation_rate | -0.34 | -0.22 | -0.16 |
| separation | -0.26 | -0.22 | -0.05 |
| hetero_ltd | +0.23 | +0.20 | +0.14 |
| presynaptic_bound | +0.22 | +0.11 | +0.09 |
| consolidated_budget | -0.14 | -0.05 | -0.05 |
| soft_bound | -0.14 | -0.06 | +0.04 |
| spread_plastic | +0.13 | +0.04 | +0.02 |
| plastic_budget | +0.11 | +0.04 | +0.11 |
| learning_rate | -0.10 | -0.07 | -0.06 |
| istdp_target | +0.09 | +0.12 | -0.08 |
| mode_tau | +0.08 | +0.01 | +0.00 |
| line_recency | -0.07 | +0.03 | +0.04 |
| istdp_rate | -0.06 | +0.02 | +0.07 |
| order_tau | +0.06 | -0.04 | -0.07 |
| channel_winners3 | -0.04 | -0.02 | -0.01 |
| assembly_inhibition | -0.03 | -0.13 | +0.08 |
| fire_threshold3 | +0.03 | -0.01 | -0.03 |
| covariance | +0.03 | +0.03 | +0.10 |
| fatigue_gain3 | +0.02 | -0.06 | +0.11 |
| winners3 | +0.00 | +0.02 | -0.01 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.