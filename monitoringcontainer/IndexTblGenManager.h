class IndexTblGenManager {
    public:
        static void InitIndexTblGenManager(){
            GetInstance().initIndexTblGenManager();
        }
    private:
        
        static IndexTblGenManager& GetInstance(){
            static IndexTblGenManager _instance;
            return _instance;
        }   
        
        int initIndexTblGenManager();
}