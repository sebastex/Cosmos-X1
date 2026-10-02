# Evolution monitor (generation 14 done, 34.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c99 | 9.23 | 8 | 7.0 | 92 | soft_bound 1.0->0.887, presynaptic_bound 1.0->0.86613, order_gain 4.0->5.52885, line_recency 0.6->0.54086, winners3 3->4, channel_winners3 4->3, separation 3.0->0.5762 |
| c92 | 9.16 | 2 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, covariance 1.0->0.81504, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.26742, order_tau 5.0->3.77467, mode_tau 30.0->17.29942, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->2.6822, spread_plastic 0.5->0.4135 |
| c112 | 9.15 | 4 | 7.0 | 94 | learning_rate 0.04->0.03466, plastic_budget 4.0->4.7963, soft_bound 1.0->0.98407, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.99395, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.44416, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.07424, fire_threshold3 0.0256->0.01702, line_recency 0.6->0.66799, winners3 3->4, channel_winners3 4->3, separation 3.0->3.28214, spread_plastic 0.5->0.1191 |
| c89 | 9.12 | 4 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->17.38749, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.4135 |
| c123 | 9.11 | 2 | 7.0 | 88 | learning_rate 0.04->0.03266, plastic_budget 4.0->4.7963, covariance 1.0->0.81504, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.83385, order_gain 4.0->3.26742, order_tau 5.0->3.77467, mode_tau 30.0->17.29942, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.5669, winners3 3->4, channel_winners3 4->3, separation 3.0->2.6822, spread_plastic 0.5->0.4192 |
| c105 | 9.08 | 2 | 7.0 | 88 | soft_bound 1.0->0.75042, hetero_ltd 1.0->0.80485, istdp_rate 50.0->74.60277, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.97447, order_gain 4.0->4.7958, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.02495, winners3 3->5, channel_winners3 4->3, separation 3.0->4.07163 |
| c117 | 9.07 | 2 | 7.0 | 88 | learning_rate 0.04->0.035, soft_bound 1.0->0.887, presynaptic_bound 1.0->0.86613, hetero_ltd 1.0->0.78286, istdp_rate 50.0->47.74066, istdp_target 0.02->0.01905, order_gain 4.0->5.52885, line_recency 0.6->0.54086, winners3 3->5, channel_winners3 4->3, separation 3.0->0.5762 |
| c94 | 9.06 | 6 | 6.8 | 96 | learning_rate 0.04->0.03466, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.98592, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.01702, line_recency 0.6->0.66799, winners3 3->4, channel_winners3 4->3, separation 3.0->3.28214, spread_plastic 0.5->0.1191 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 1 | 24 |
| capacity | 6 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 0 | 24 |
| continual | 6 | 24 |
| retention | 8 | 24 |
| order | 1 | 24 |

- A 40% cue brings back on average 63% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): q>a: 3, q>t: 1, q>e: 1, w>a: 1, a>t: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 15, light>house: 1, green>candy: 1, cloud>house: 1, apple>dream: 1, light>tiger: 1, green>river: 1, cloud>candy: 1

## Picture so far (all generations together)

- Brain tests so far: 360 (124 versions, generation 15).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 35 | 360 | 0 | 30 |
| capacity | 57 | 360 | 3 | 30 |
| efficiency | 13 | 360 | 1 | 30 |
| wordpairs | 46 | 360 | 1 | 30 |
| continual | 69 | 360 | 4 | 30 |
| retention | 79 | 360 | 3 | 30 |
| order | 2 | 360 | 0 | 30 |

- Old memories getting worse (all tests): wiped out -0.015, pushed aside by new memories +0.012 on average; of 69 failed 'keep old memories' tests, 51 were mainly pushed aside (interference), 18 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 2399, stored but NOT recalled 466, not stored 15.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=16); c1 64 words 39% (n=16); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c112 32 words 78% (n=2); c112 64 words 52% (n=2); c114 32 words 72% (n=2); c114 64 words 45% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c36 32 words 59% (n=2); c36 64 words 30% (n=2); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 58% (n=4); c46 64 words 40% (n=4); c48 32 words 56% (n=2); c48 64 words 34% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); c76 32 words 78% (n=2); c76 64 words 34% (n=2); c79 32 words 75% (n=4); c79 64 words 46% (n=4); c83 32 words 75% (n=2); c83 64 words 41% (n=2); c89 32 words 91% (n=2); c89 64 words 52% (n=2); c94 32 words 91% (n=4); c94 64 words 55% (n=4); c95 32 words 69% (n=2); c95 64 words 38% (n=2); c96 32 words 81% (n=2); c96 64 words 52% (n=2); c99 32 words 84% (n=6); c99 64 words 49% (n=6); prime 32 words 70% (n=30); prime 64 words 31% (n=30)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| inhibition_radius3 | -0.42 | -0.35 | -0.22 |
| encoding_suppression | +0.40 | +0.28 | +0.28 |
| consolidation_rate | -0.33 | -0.20 | -0.13 |
| order_gain | +0.32 | +0.20 | +0.15 |
| separation | -0.20 | -0.16 | -0.08 |
| hetero_ltd | +0.14 | +0.12 | +0.08 |
| learning_rate | -0.14 | -0.09 | -0.08 |
| spread_plastic | +0.11 | +0.06 | +0.03 |
| winners3 | +0.11 | +0.14 | +0.07 |
| soft_bound | -0.10 | -0.03 | -0.00 |
| line_recency | -0.09 | +0.01 | +0.03 |
| plastic_budget | +0.08 | +0.03 | +0.10 |
| consolidated_budget | -0.08 | +0.03 | -0.01 |
| mode_tau | +0.07 | -0.01 | +0.02 |
| assembly_inhibition | -0.06 | -0.13 | +0.05 |
| istdp_target | +0.06 | +0.08 | -0.05 |
| presynaptic_bound | +0.05 | -0.00 | +0.03 |
| covariance | +0.03 | +0.02 | +0.08 |
| channel_winners3 | -0.03 | +0.02 | -0.03 |
| fatigue_gain3 | -0.02 | -0.04 | +0.02 |
| order_tau | -0.02 | -0.12 | -0.10 |
| istdp_rate | -0.01 | +0.07 | +0.11 |
| fire_threshold3 | -0.01 | -0.03 | -0.04 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.