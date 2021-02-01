#!/bin/bash
protoc --cpp_out=. ./scenerunnermanager.proto
protoc --cpp_out=. ./runnerregister.proto
protoc --cpp_out=. ./runneraddscene.proto
