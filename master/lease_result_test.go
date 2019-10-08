package kea

import (
	"testing"

	ut "cement/unittest"
	"github.com/golang/protobuf/proto"
)

func TestLeaseResult(t *testing.T) {
	result := ToLeaseResult(nil)
	resultData, err := proto.Marshal(&result)
	ut.Equal(t, err, nil)
	ut.Assert(t, len(resultData) != 0, "marshal result data len must not be 0")
}
