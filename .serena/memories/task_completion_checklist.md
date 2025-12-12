# Task Completion Checklist

When completing a task in the NBox project, follow these steps:

## 1. Code Implementation
- [ ] Implement the feature following TDD approach (test first, then code)
- [ ] Follow KISS principle - keep it simple
- [ ] Use existing code style and naming conventions
- [ ] Add minimal necessary comments

## 2. Build and Compile
```bash
cmake --build build
```
- [ ] Ensure no compilation errors
- [ ] Ensure no warnings

## 3. Testing
- [ ] Run relevant tests (when test framework is set up)
- [ ] For now: manual testing with appropriate macro files
```bash
./build/nbox_sim -m test.mac
```

## 4. Verification
- [ ] Check output files are generated correctly
- [ ] Verify ROOT output format (if applicable)
- [ ] Test with different configuration files

## 5. Documentation
- [ ] Update plan files in TODO/ if implementation differs from plan
- [ ] Update memory files if major architecture changes
- [ ] No need for excessive documentation (KISS)

## 6. Integration
- [ ] Ensure changes work with multi-threading
- [ ] Test with `-m run.mac` for batch mode
- [ ] Test interactive mode (no arguments)

## Common Issues to Check
- [ ] ROOT file closing properly (no memory leaks)
- [ ] JSON parsing error handling
- [ ] Thread safety (read-only after init)
- [ ] Geant4 units used consistently
- [ ] Proper cleanup in destructors

## When Feature is Complete
- [ ] Run full simulation test
- [ ] Verify output format matches requirements
- [ ] Commit changes (if using git)
- [ ] Mark task as done in plan file
