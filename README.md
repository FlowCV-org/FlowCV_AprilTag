# FlowCV AprilTag Detector

Adds [AprilTag](https://april.eecs.umich.edu/software/apriltag) Dectection to [FlowCV](https://github.com/FlowCV-org/FlowCV)

---

### Build Instruction

Prerequisites:
* Clone [FlowCV](https://github.com/FlowCV-org/FlowCV) repo.

Build Steps:
1. Clone this repo.
2. cd to the repo directory
3. Run the following commands:

```shell
mkdir Build
cd Build
cmake .. -DFlowCV_DIR=/path/to/FlowCV/FlowCV_SDK
make
```
