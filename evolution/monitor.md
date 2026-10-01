# Evolution monitor (generation 4 done, 32.6 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c33 | 9.02 | 4 | 6.8 | 94 | learning_rate 0.04->0.06267, plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.73385, istdp_rate 50.0->74.10519, istdp_target 0.02->0.02421, consolidation_rate 0.0->0.02862, consolidated_budget 0.5->0.86684, assembly_inhibition 0.0->0.00063, mode_tau 30.0->25.67333, fatigue_gain3 0.0638->0.07178, fire_threshold3 0.0256->0.03678, winners3 3->4, channel_winners3 4->3 |
| c11 | 8.88 | 10 | 6.6 | 96 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| c29 | 8.70 | 4 | 6.5 | 94 | plastic_budget 4.0->4.16408, istdp_target 0.02->0.02722, fire_threshold3 0.0256->0.02666, winners3 3->4 |
| c3 | 8.68 | 4 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c27 | 8.67 | 2 | 6.5 | 88 | learning_rate 0.04->0.0391, covariance 1.0->0.89106, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917, channel_winners3 4->3 |
| c16 | 8.66 | 2 | 6.5 | 94 | plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidation_rate 0.0->0.00569, assembly_inhibition 0.0->0.00063, fire_threshold3 0.0256->0.03678 |
| c32 | 8.65 | 2 | 6.5 | 88 | learning_rate 0.04->0.04686, plastic_budget 4.0->4.34164, fatigue_gain3 0.0638->0.10403 |
| prime | 8.64 | 10 | 6.4 | 95 | (Prime) |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 6 | 24 |
| efficiency | 4 | 24 |
| wordpairs | 0 | 24 |
| continual | 8 | 24 |
| retention | 12 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 78% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): k>t: 3, q>t: 1, k>e: 1, m>a: 1, q>k: 1, w>a: 1, a>e: 1, k>q: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 6, cloud>candy: 2, light>river: 1, smile>mouse: 1, apple>tiger: 1, green>candy: 1, smile>river: 1

## Picture so far (all generations together)

- Brain tests so far: 120 (44 versions, generation 5).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 9 | 120 | 0 | 10 |
| capacity | 24 | 120 | 0 | 10 |
| efficiency | 9 | 120 | 2 | 10 |
| wordpairs | 2 | 120 | 0 | 10 |
| continual | 34 | 120 | 2 | 10 |
| retention | 50 | 120 | 2 | 10 |
| order | 1 | 120 | 0 | 10 |

- Old memories getting worse (all tests): wiped out -0.015, pushed aside by new memories +0.036 on average; of 22 failed 'keep old memories' tests, 19 were mainly pushed aside (interference), 3 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 532, stored but NOT recalled 44, not stored 0.
- Big-load probe (true word recall %, mean over brains): c11 32 words 74% (n=6); c11 64 words 36% (n=6); c21 32 words 66% (n=2); c21 64 words 39% (n=2); c26 32 words 69% (n=2); c26 64 words 34% (n=2); c29 32 words 72% (n=2); c29 64 words 38% (n=2); c3 32 words 81% (n=2); c3 64 words 36% (n=2); c33 32 words 69% (n=2); c33 64 words 41% (n=2); c9 32 words 69% (n=2); c9 64 words 22% (n=2); prime 32 words 70% (n=6); prime 64 words 36% (n=6)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.38 | +0.16 | +0.29 |
| inhibition_radius3 | -0.23 | -0.16 | -0.08 |
| plastic_budget | -0.22 | -0.19 | -0.15 |
| winners3 | +0.21 | +0.16 | +0.12 |
| covariance | +0.20 | +0.06 | +0.15 |
| consolidation_rate | -0.14 | -0.17 | -0.11 |
| istdp_rate | +0.14 | +0.17 | +0.05 |
| soft_bound | +0.13 | +0.04 | +0.10 |
| istdp_target | +0.12 | -0.03 | +0.12 |
| fire_threshold3 | +0.11 | +0.05 | -0.04 |
| order_tau | +0.11 | -0.00 | +0.13 |
| channel_winners3 | +0.10 | +0.17 | +0.09 |
| hetero_ltd | +0.10 | +0.13 | +0.11 |
| fatigue_gain3 | +0.08 | +0.09 | +0.06 |
| order_gain | +0.08 | -0.01 | +0.12 |
| presynaptic_bound | -0.07 | +0.02 | -0.10 |
| assembly_inhibition | -0.05 | -0.13 | +0.04 |
| mode_tau | +0.04 | +0.05 | -0.01 |
| line_recency | +0.04 | +0.08 | -0.05 |
| learning_rate | +0.02 | -0.07 | +0.05 |
| consolidated_budget | -0.00 | +0.08 | +0.07 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.