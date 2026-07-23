# Engineering Constitution
## Production Engineering Standards for C++, DirectX9, Game Engines, Reverse Engineering, and AI-Assisted Development

> **Purpose:** This document defines the governing engineering standards for all repositories within this organization. It is intended to serve as the authoritative specification for both human developers and AI coding assistants (Claude, ChatGPT, Copilot, etc.).

# Table of Contents

1. Engineering Philosophy
2. Core Principles (DRY, KISS, YAGNI, SOLID)
3. Software Craftsmanship
4. Repository Organization
5. C++ Standards
6. Modern C++ Features
7. Memory Ownership (RAII)
8. Error Handling
9. API Design
10. Design Patterns
11. Anti-Patterns
12. Rendering Architecture
13. DirectX9 Standards
14. Shader Development
15. Resource Management
16. Asset Pipelines
17. Binary File Parsers
18. Reverse Engineering
19. Hooking Standards
20. Performance Engineering
21. Multithreading
22. Data-Oriented Design
23. Logging
24. Testing
25. Documentation
26. Security
27. Static Analysis
28. Build Systems
29. Git Workflow
30. Code Review
31. Technical Debt
32. AI Coding Rules
33. Claude AI Enforcement Checklist
34. Project Completion Checklist

# Chapter 1: DRY

## Purpose
Every piece of knowledge shall exist in exactly one authoritative location.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 2: KISS

## Purpose
Prefer the simplest architecture that satisfies current requirements.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 3: YAGNI

## Purpose
Never implement speculative features.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 4: SOLID

## Purpose
Apply all five SOLID principles where they improve maintainability.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 5: RAII

## Purpose
Every owned resource must have deterministic lifetime.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 6: Composition over Inheritance

## Purpose
Favor composable systems.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 7: High Cohesion, Low Coupling

## Purpose
Modules should have focused responsibilities.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 8: Boy Scout Rule

## Purpose
Leave code cleaner than it was found.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 9: Law of Demeter

## Purpose
Limit knowledge between collaborating objects.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 10: Tell Don't Ask

## Purpose
Encapsulate behavior with data.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 11: Engineering Standard 11

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 12: Engineering Standard 12

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 13: Engineering Standard 13

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 14: Engineering Standard 14

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 15: Engineering Standard 15

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 16: Engineering Standard 16

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 17: Engineering Standard 17

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 18: Engineering Standard 18

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 19: Engineering Standard 19

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 20: Engineering Standard 20

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 21: Engineering Standard 21

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 22: Engineering Standard 22

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 23: Engineering Standard 23

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 24: Engineering Standard 24

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 25: Engineering Standard 25

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 26: Engineering Standard 26

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 27: Engineering Standard 27

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 28: Engineering Standard 28

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 29: Engineering Standard 29

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 30: Engineering Standard 30

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 31: Engineering Standard 31

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 32: Engineering Standard 32

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 33: Engineering Standard 33

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 34: Engineering Standard 34

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 35: Engineering Standard 35

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 36: Engineering Standard 36

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 37: Engineering Standard 37

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 38: Engineering Standard 38

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 39: Engineering Standard 39

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 40: Engineering Standard 40

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 41: Engineering Standard 41

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 42: Engineering Standard 42

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 43: Engineering Standard 43

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 44: Engineering Standard 44

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 45: Engineering Standard 45

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 46: Engineering Standard 46

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 47: Engineering Standard 47

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 48: Engineering Standard 48

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 49: Engineering Standard 49

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 50: Engineering Standard 50

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 51: Engineering Standard 51

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 52: Engineering Standard 52

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 53: Engineering Standard 53

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 54: Engineering Standard 54

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 55: Engineering Standard 55

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 56: Engineering Standard 56

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 57: Engineering Standard 57

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 58: Engineering Standard 58

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 59: Engineering Standard 59

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 60: Engineering Standard 60

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 61: Engineering Standard 61

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 62: Engineering Standard 62

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 63: Engineering Standard 63

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 64: Engineering Standard 64

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 65: Engineering Standard 65

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 66: Engineering Standard 66

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 67: Engineering Standard 67

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 68: Engineering Standard 68

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 69: Engineering Standard 69

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 70: Engineering Standard 70

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 71: Engineering Standard 71

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 72: Engineering Standard 72

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 73: Engineering Standard 73

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 74: Engineering Standard 74

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 75: Engineering Standard 75

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 76: Engineering Standard 76

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 77: Engineering Standard 77

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 78: Engineering Standard 78

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 79: Engineering Standard 79

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 80: Engineering Standard 80

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 81: Engineering Standard 81

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 82: Engineering Standard 82

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 83: Engineering Standard 83

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 84: Engineering Standard 84

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 85: Engineering Standard 85

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 86: Engineering Standard 86

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 87: Engineering Standard 87

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 88: Engineering Standard 88

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 89: Engineering Standard 89

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 90: Engineering Standard 90

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 91: Engineering Standard 91

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 92: Engineering Standard 92

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 93: Engineering Standard 93

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 94: Engineering Standard 94

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 95: Engineering Standard 95

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 96: Engineering Standard 96

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 97: Engineering Standard 97

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 98: Engineering Standard 98

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 99: Engineering Standard 99

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible


# Chapter 100: Engineering Standard 100

## Purpose
This chapter defines production-grade architectural rules, rationale, examples, review criteria, common pitfalls, enforcement requirements for AI-generated and human-authored code.

## Philosophy
All code shall prioritize correctness, maintainability, readability, determinism, scalability, debuggability, portability where applicable, and long-term sustainability.

## Mandatory Rules
- Design for clarity before optimization.
- Favor explicit ownership and lifetime management.
- Avoid undefined behavior.
- Document invariants and assumptions.
- Write self-documenting code through meaningful names.
- Separate interfaces from implementations.
- Eliminate duplicated logic.
- Keep modules cohesive and loosely coupled.
- Treat warnings as errors.
- Prefer compile-time validation.
- Benchmark before optimizing.

## AI Enforcement
When generating code:
1. Reject duplicated logic.
2. Reject global mutable state unless justified.
3. Require RAII.
4. Require const-correctness.
5. Require exception safety or explicit error handling.
6. Prefer composition.
7. Explain architectural decisions.
8. Produce production-ready documentation.

## Review Checklist
- [ ] Correct
- [ ] Maintainable
- [ ] Readable
- [ ] Tested
- [ ] Documented
- [ ] Performant
- [ ] Secure
- [ ] Extensible

# Final Constitution

This constitution supersedes convenience. Every contribution shall improve the codebase's long-term health. AI assistants are expected to follow these standards by default, explaining any deliberate deviations and documenting trade-offs.
