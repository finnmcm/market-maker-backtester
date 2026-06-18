---
name: quiz
description: Quiz the author on a topic or on what they just built, to verify real understanding. Usage: /quiz <topic>
argument-hint: [topic]
---

Quiz me on: $ARGUMENTS
(If no topic is given, quiz me on the code I most recently changed.)

- Ask 4-6 questions, one at a time, escalating in difficulty. Wait for my answer
  before asking the next.
- Mix recall, application, and "why" questions. At least one should probe a
  common misconception.
- After each answer, tell me if I'm right and fill the gap if I'm not -- but do
  not hand me code.
- Finish with a one-line read on what I clearly know and what I should review.
