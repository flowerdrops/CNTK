#ifndef CORE_GRAPH_GRAPH_H
#define CORE_GRAPH_GRAPH_H

#include <string>

#pragma warning(push)
#pragma warning(disable : 4800 4610 4512 4510 4267 4127 4125 4100 4456)
#include "./protobuf/graph.pb.h"
#pragma warning(pop)

#include "ONNXStatus.h"

namespace CommonIR
{
    typedef DataProto_DenseTensorProto DenseTensorProto;
    typedef uint32_t NODEINDEX;
    typedef int64_t GRAPH_VERSION;
    typedef std::unordered_map<std::string, AttributeProto> NodeAttributes;

    class Graph;

    // Node argument definition, for both input and output,
    // including arg name, arg type and arg shape.
    class NodeArg
    {
    public:

        // Constructor by specifying a name, type and shape.
        NodeArg(const std::string& p_name, const TypeProto& p_type,
            const TensorShapeProto& p_shape);

        // Get node arg name.
        const std::string& Name() const;

        // Get node arg type.
        const TypeProto& Type() const;

        // Get node arg shape.
        const TensorShapeProto& Shape() const;

        const NodeProto_InputOutputProto& ToProto() const;

    private:

        friend class Node;

        // Constructor by specifying a <NodeProto_InputOutputProto>.
        // This is called when loading a <Graph> from <GraphProto> normally.
        NodeArg(const NodeProto_InputOutputProto& p_nodeProtoInputOutput);

        NodeProto_InputOutputProto m_nodeArgData;
    };

    // Function representation.
    // There're two cases of instantiating a function,
    // 1. Instantiating a function instance per <FunctionDefProto> which does
    //    not contain template.
    // 2. Instantiating a function instance per <Node> and <FunctionDefProto>
    //    which contains template. Node's inputs contain type value to do
    //    instantiation.
    class Function
    {
    public:

        // Get function body - a subgraph.
        // Returned pointer owned by <*this> Function.
        // The <Graph*> returned is a graph definition. To run it, a <Node>
        // refers to <*this> Function needs to be fed since all inputs/outputs'
        // actual parameters are in <Node>.
        Graph* Body();

        // Get function name.
        // A function's name could be either its function definition name
        // m_functionDefProto.name(), or m_functionDefProto.name() + template
        // argument value.
        const std::string& Name();

        // Get the protobuf representation of <*this> function.
        // TODO: Flag may be added to indicate whether <*this> function body is
        // changed due to optimization, if yes, the <m_functionDefProto> should
        // be synced with <*this> function body.
        const FunctionDefProto& Proto();

    private:

        friend class Graph;

        Function() = delete;

        // Constructor.
        // <p_node> specifies the node that refers to <*this> function. It's
        // used to instantiate <p_funcProto> if <p_funcProto> is a function
        // template.
        // <p_funcProto> specifies a function definition that a node refers to.
        // <p_version> specifies a graph version this function will use for its
        // function body - a subgraph. It normally uses the same version as its
        // parent graph.
        Function(Node* p_node,
            const FunctionDefProto& p_funcProto,
            GRAPH_VERSION p_version);

        // Function body which is a SubGraph.
        std::unique_ptr<Graph> m_body;

        Node* m_node;
        std::string m_name;

        // Function definition, which should be synced with <m_body> if there's
        // any optimization on <m_body>.
        FunctionDefProto m_functionDefProto;
    };

    // A node representation class.
    class Node {

    public:

        // An edge end. It could be input or output edge end of a node.
        // For node's input edge end, it's the source end, as the destination
        // end is the node itself.
        // For node's ouput edge end, it's the destination end, as the source
        // end is the node itself.
        class EdgeEnd
        {
        public:

            // Constructor.
            // An EdgeEnd contains a Node pointer, a NodeArg pointer.
            // NOTE: it does not own the Node pointer and NodeArg pointer.
            EdgeEnd(const Node& p_node, const NodeArg& p_nodeArg);

            // Get the <Node*> that this edge end refers to. 
            const Node* GetNode() const;

            // Get the <NodeArg*> that this edge end refers to.
            const NodeArg* GetNodeArg() const;

        private:

            const Node* m_node;

            const NodeArg* m_nodeArg;
        };

        // An iterator helper class for iterating a Node's neighbour nodes.
        class NodeConstIterator
        {
        public:

            NodeConstIterator(std::set<const Node*>::const_iterator p_iter);

            bool operator==(const NodeConstIterator& p_other) const;

            bool operator!=(const NodeConstIterator& p_other) const;

            void operator++();

            const Node* operator*();

        private:

            std::set<const Node*>::const_iterator m_iter;
        };

        // Get node index.
        NODEINDEX Index() const;

        // Get node name.
        const std::string& Name() const;

        // Get node operator type.
        const std::string& OpType() const;

        // Read/Write <*this> node's input args' definition, including name,
        // type and shape.
        const std::vector<std::vector<NodeArg>>& InputDefs() const;
        std::vector<std::vector<NodeArg>>& Mutable_InputDefs();

        // Read/Write <*this> node's output args' definition, including name,
        // type and shape.
        const std::vector<NodeArg>& OutputDefs() const;
        std::vector<NodeArg>& Mutable_OutputDefs();

        // Functions defined to traverse a Graph as below.
        // Read all input nodes of <*this>.
        Node::NodeConstIterator InputNodes_begin() const;
        Node::NodeConstIterator InputNodes_end() const;
        // Read all output nodes of <*this>.
        Node::NodeConstIterator OutputNodes_begin() const;
        Node::NodeConstIterator OutputNodes_end() const;
        // Given input arg, get the source end of an input edge.
        bool InputEdgeSrcEnd(NodeArg* p_inputArg,
            /*out*/const EdgeEnd** p_inputEdgeSrcEnd);

        // Add a node attribute with specified attribute name and value.
        template <typename T>
        bool AddAttribute(const std::string& p_attrName, const T& p_value)
        {
            // TODO: add implementation.
        }

        // Clear specified node attribute.
        bool ClearAttribute(const std::string& p_attrName);

        // Get node attributes.
        const NodeAttributes& GetAttributes() const;

        // Indicates on which we will run this node in runtime.        
        // Executor will decide which device that this node will run against
        // and set it properly.
        // TODO: may change the return value type to be an ENUM.
        const std::string& Device() const;
        void SetDevice(const std::string& p_device);

        // Get the corresponding <NodeProto>.
        void ToProto(NodeProto& p_proto) const;

    private:

        friend class Graph;

        // Node could ONLY be constructed and owned by a <Graph>.
        Node() {}
        Node(NODEINDEX p_index, Graph* p_graph)
            : m_index(p_index),
            m_graph(p_graph) {}
        Node(const Node& p_other);

        void Init(const NodeProto& p_nodeProto);
        void Init(const std::string& p_name,
            const std::string& p_opType,
            const std::vector<std::vector<NodeArg>>& p_inputArgs,
            const std::vector<NodeArg>& p_outputArgs);

        // Node index.
        NODEINDEX m_index;

        // Node name.
        std::string m_name;

        // Node operator type.
        std::string m_opType;

        // Node inputs' definition.
        std::vector<std::vector<NodeArg>> m_inputDefs;

        // Node outputs' definition.
        std::vector<NodeArg> m_outputDefs;

        // Node inputs' instantiation.
        std::unordered_map<const NodeArg*, EdgeEnd> m_inputs;
        // Node input nodes, besides input nodes mentioned in <m_inputs> above,
        // it also contains all control input nodes;
        std::set<const Node*> m_inputNodes;
        // Control input nodes' names.
        std::set<std::string> m_controlInputs;
        // Node's output nodes.
        std::set<const Node*> m_outputNodes;

        // Device.
        std::string m_device;

        // Map from attribute name to attribute.
        // This allows attribute adding and removing.
        NodeAttributes m_attributes;

        Graph* m_graph;
    };

    // A graph representation class.
    class Graph
    {
    public:

        // An iterator helper to access graph nodes without copy.
        // The iterator itself does not own any data.
        class NodeIterator
        {
        public:

            // Constructor.
            NodeIterator(NODEINDEX p_currentNodeIndex, Graph* p_graph)
                : m_graph(p_graph),
                m_currentNodeIndex(p_currentNodeIndex)
            {
            }

            bool operator==(const NodeIterator& p_other) const;

            bool operator!=(const NodeIterator& p_other) const;

            void operator++();

            Node* operator*();

        private:

            Graph* m_graph;

            // it's the Node Index in <m_nodes> of the <m_graph>.
            NODEINDEX m_currentNodeIndex;
        };

        // Constructor from scratch.
        Graph(const std::string& p_name, GRAPH_VERSION p_version);

        // Constructor: Given a <GraphProto> loaded from model file, construct
        // a <Graph> object.
        Graph(const GraphProto& p_graphProto);

        // Constructor: Given a function definition and a node which refers to
        // the function, construct a <Graph> object.
        // Normally the <p_name> could be the parent node name and the
        // <p_version> could be the parent graph's version.
        // Question: will a node defined in a function refers another function
        // please? I (Ke) am assuming we don't allow such case here for now.
        Graph(Node* p_node,
            const FunctionDefProto& p_functionProto,
            const std::string& p_name,
            GRAPH_VERSION p_version);

        // Resolve <*this> graph to ensure it's in a good shape with full
        // functionality.
        // 1. Run through all validation rules.
        //    a. Node name and node output's names should be unique.
        //    b. Attribute match between node and op definition.
        //    c. Input/Output match between node and op definition.
        //    d. Graph is acyclic.
        // 2. Check & Setup inner nodes' dependency.
        // 3. Cleanup function definition lists.
        // Returns resolving status.
        Status Resolve();

        // Getter and Setter for <m_version>.
        GRAPH_VERSION Version() const;
        void SetVersion(GRAPH_VERSION p_version);

        // Getter and Setter for <m_name>.
        const std::string& Name() const;
        void SetName(const std::string& p_name);

        // Getter and Setter for graph parameters.
        bool GetParamter(const std::string& p_paramName,
            DenseTensorProto& p_value) const;
        void SetParameter(const std::string& p_paramName,
            const DenseTensorProto& p_value);

        // Add or Remove a function definition.
        bool AddFunctionDef(const FunctionDefProto& p_function);
        void RemoveFunctionDef(const std::string& p_functionName);

        // Get node given specific node index.
        Node* GetNode(NODEINDEX p_nodeIndex);

        // Get node iterator to access all effective nodes in the graph.
        Graph::NodeIterator Nodes_begin();
        Graph::NodeIterator Nodes_end();

        // Max Node Index.
        NODEINDEX MaxNodeIndex() const;

        // Number of nodes in the <Graph>.
        // This is smaller than MaxNodeIndex(), since there may be nodes
        // removed during optimization.
        int NumberOfNodes();

        // Add, remove node from <*this> graph.
        Node* AddNode(const std::string& p_name,
            const std::string& p_opType,
            const std::vector<std::vector<NodeArg>>& p_inputArgs,
            const std::vector<NodeArg>& p_outputArgs);
        Node* AddNode(const Node& p_other);
        bool RemoveNode(NODEINDEX p_nodeIndex);

        // Add control edge into <*this> graph.
        // The <p_dstNodeIndex> node does not consume any data output by
        // <p_srcNodeIndex>, but it's designed to be executed behind.
        bool AddControlEdge(NODEINDEX p_srcNodeIndex, NODEINDEX p_dstNodeIndex);

        // Try to get function with specified <p_nodeIndex>. Return true if the
        // specified node refers to a function, and <p_function> will be the 
        // function; false otherwise, and <p_function> will be unchanged.
        bool TryGetFunction(NODEINDEX p_nodeIndex,
            /*out*/ Function** p_function);

        // Serialize the <Graph> into <GraphProto>.
        const GraphProto& Proto();

        // Inline all function in <*this> and construct <p_graph>
        // without any functions. <p_graph> owned by caller.
        bool InlineAllFunctions(/*out*/Graph* p_graph) const;

        bool IsSourceNode(NODEINDEX p_index) const;
        bool IsSinkNode(NODEINDEX p_index) const;

        const Node* SourceNode() const;
        const Node* SinkNode() const;

    private:

        friend class Node;

        Node* AllocateNode();
        void ReleaseNode(NODEINDEX p_nodeIndex);

        // Add node with specified <p_nodeProto>.
        Node* AddNode(const NodeProto& p_nodeProto);

        Status VerifyNoDuplicateName(/*out*/
            std::unordered_map<std::string, Node::EdgeEnd>& p_outputArgs);

        // Verify node op_type should be valid, either pointing to an
        // operator or a function.
        // Verify node attributes should match operator/function attributes.
        // Verify node inputs/outputs' type/shape should match operator/
        // function inputs/outputs type/shape.
        Status VerifyNodeAndOpMatch(
            /*out*/ std::set<std::string>& p_funcDefNames);

        // Build and verify node connection (edges).
        // Verify NodeArg name/type/shape matching correctly.
        Status BuildConnections(
            const std::unordered_map<std::string, Node::EdgeEnd>& p_outputArgs);

        // Check whether <*this> graph is acyclic.
        // Depth-first going thru the graph and check whether there's any back
        // edge.
        Status Graph::CheckIsAcyclic();

        Status DepthFirstAccess(std::unordered_set<NODEINDEX> p_ancestors,
            NODEINDEX p_current,
            std::unordered_set<NODEINDEX>& p_visitedNodes);

        // Clean function definition map.
        // Remove function definitions not refered by any node.
        void CleanFunctionDefMap(const std::set<std::string>& p_funcDefNames);

        // Add source/sink nodes to <*this> graph.
        void AddSourceSinkNodes();

        // Graph nodes.
        // Element in <m_nodes> may be nullptr due to graph optimization.
        std::vector<std::unique_ptr<Node>> m_nodes;

        // Number of nodes.
        // Normally this is smaller than the size of <m_nodes>, as some
        // elements in <m_nodes> may be removed when doing graph optimization,
        // or some elements may be merged, etc.
        int m_numOfNodes;

        NODEINDEX m_sourceNodeIndex;
        NODEINDEX m_sinkNodeIndex;

        // GraphProto to store name, version, parameters.
        // When serilizing <*this> Graph to a GraphProto, the nodes and
        // functions in <Graph> will also be fed into <m_graphProto> so that
        // it's consistent with <*this> graph.
        GraphProto m_graphProto;

        // The node which refers to <*this> graph (Function).
        Node* m_node;

        // Graph function instantiations.
        std::unordered_map<std::string,
            std::unique_ptr<Function>> m_functionMap;

        // Graph function definitions.
        std::unordered_map<std::string, FunctionDefProto> m_funcDefMap;

        // A flag indicates whether <*this> graph needs to be resolved.
        bool m_isGraphValid;
    };
}

#endif  // COMMONIR_GRAPH_H